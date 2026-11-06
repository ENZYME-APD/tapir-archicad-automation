using GH_IO.Serialization;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.ToolSettings;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // One component per Archicad tool (Walls, Slabs, ...) that mirrors the
    // tool's settings dialog: one JSON input and output per dialog panel,
    // keyed by the labels of the dialog's controls, built from the tool's
    // ToolSettings/<Tool>.json table. Three buttons:
    //   Refresh  reads the elements (GetDetailsOfElements),
    //   Execute  writes the given settings to them (the tool's Modify command,
    //            SetDetailsOfElements and the Element ID property),
    //   Create   makes new elements from the geometry inputs and the same JSON;
    //            in a tool with CanUpdateCreatedElements it updates the
    //            elements it made before instead (saved with the .gh file).
    // Writing and creating only happen on a button press, never through
    // AutoExecute. Deleting elements is left to the DeleteElements component
    // on purpose. See CLAUDE.md, "Archicad tool components".
    public abstract class ElementToolComponent : Component, IButtonComponent
    {
        private const int ElementGuidIndex = 0;
        private const int ElementIdIndex = 1;
        private const int LayerIndexIndex = 2;
        private const int DrawOrderIndex = 3;
        private const int FavoriteNameIndex = 4;
        private const int ErrorsOutputIndex = 4;
        private const int FirstGeometryIndex = 5;

        private const string ElementIdPropertyName = "General_ElementID";

        private enum ToolAction
        {
            None,
            Refresh,
            Execute,
            Create
        }

        private bool _refreshRequested;
        private bool _executeRequested;
        private bool _createRequested;
        private ToolAction _action;

        // The elements Create made: one list per solve iteration (a branch of
        // the geometry tree), one guid per geometry item of that branch (null
        // where none was made), saved with the .gh file: the next Create
        // updates them instead of making new ones (#521). Keyed by the
        // iteration as well as the item, so two branches never share an
        // element. _createdBy is the component they belong to, so a pasted copy
        // starts without them and two components never drive the same elements.
        private List<List<string>> _createdGuids = new List<List<string>>();
        private Guid _createdBy = Guid.Empty;
        private bool _updateCreatedElements = true;

        private const string UpdateCreatedElementsText = "Create Updates the Elements Created Here";
        private const string ForgetCreatedElementsText = "Forget the Elements Created Here";
        private const string UpdateCreatedElementsKey = "UpdateCreatedElements";
        private const string CreatedElementCountKey = "CreatedElementCount";
        private const string CreatedElementGuidKey = "CreatedElementGuid";
        private const string CreatedElementIterationKey = "CreatedElementIteration";
        private const string CreatedElementItemKey = "CreatedElementItem";

        // True for a tool whose Modify command takes everything Create writes,
        // the geometry included, so Create can update its own elements.
        protected virtual bool CanUpdateCreatedElements => false;

        // The tool's table name (ToolSettings/<ToolName>.json).
        protected abstract string ToolName { get; }

        // True: JSON keys are the dialog labels; false: the Add-On's names.
        protected abstract bool UseLabels { get; }

        protected abstract string CreateCommand { get; }
        protected abstract string CreateArrayKey { get; }
        protected abstract string ModifyCommand { get; }
        protected abstract string ModifyArrayKey { get; }

        // The key a Modify command expects the settings of an item under,
        // next to its elementId (e.g. ModifyMeshes' meshData); null when the
        // settings are the item's own properties (e.g. ModifyWalls).
        protected virtual string ModifyItemSettingsKey => null;

        // The item keys the Create and the Modify command accept (their input
        // schemas), so a setting goes to the command that can write it.
        protected abstract ISet<string> CreateKeys { get; }
        protected abstract ISet<string> ModifyKeys { get; }

        // The keys the Modify command does not take but SetDetailsOfElements
        // does, in its typeSpecificDetails; Execute writes them there.
        protected virtual ISet<string> TypeSpecificDetailsKeys { get; } = new HashSet<string>();

        // The inputs that define the geometry, drawn in Archicad (e.g. a
        // wall's start and end point). Only Create uses them.
        protected abstract int GeometryInputCount { get; }
        protected abstract void AddGeometryInputs();
        protected abstract void AddGeometryOutputs();

        // Reads the geometry inputs into one JObject of Add-On keys per new
        // element; false (with an error added) when they are missing or invalid.
        protected abstract bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry);

        protected abstract void SetGeometryOutputs(
            IGH_DataAccess da,
            int firstOutputIndex,
            List<JObject> details);

        // The dialog panels that get a JSON input and output, in the dialog's
        // order. Saved definitions find parameters by position, so this list
        // is append-only: never reorder or remove a panel, and add new ones at
        // its end (the JSON parameters come last, after the geometry).
        // Fields of the table in another panel are not exposed.
        protected abstract IReadOnlyList<string> JsonGroups { get; }

        // Lets a tool drop or adjust settings (Add-On keys) it cannot write
        // as given, e.g. values a Refresh returns but the Modify command does
        // not take. Each note is shown once as a remark.
        protected virtual void PrepareSettings(
            JObject settings,
            ISet<string> notes)
        {
        }

        // Lets a tool drop settings (Add-On keys) of a new element that its
        // geometry inputs define, e.g. a beam's offset to its home story,
        // which the Z of its reference line sets. Called after PrepareSettings,
        // only for the elements Create makes (not for the ones it updates).
        protected virtual void PrepareCreateSettings(
            JObject settings,
            ISet<string> notes)
        {
        }

        // Lets a tool adjust a Create item after the settings were added.
        protected virtual void PrepareCreateItem(
            JObject item)
        {
        }

        private static readonly Dictionary<string, ToolSettingsTable> Tables =
            new Dictionary<string, ToolSettingsTable>();

        protected ToolSettingsTable Table
        {
            get
            {
                lock (Tables)
                {
                    if (!Tables.TryGetValue(ToolName, out var table))
                    {
                        table = ToolSettingsTable.Load(ToolName);
                        Tables[ToolName] = table;
                    }
                    return table;
                }
            }
        }

        private IReadOnlyList<string> Groups => JsonGroups;

        private int FirstGroupIndex => FirstGeometryIndex + GeometryInputCount;

        protected static string GroupParamName(
            string group)
        {
            // "Floor Plan and Section" -> FloorPlanAndSection_JSON
            var words = group.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries)
                .Select(w => string.Concat(w.Where(char.IsLetterOrDigit)))
                .Where(w => w.Length > 0)
                .Select(w => char.ToUpperInvariant(w[0]) + w.Substring(1));
            return string.Concat(words) + "_JSON";
        }

        // The group is the one Graphisoft puts the tool in: Design Tools by
        // default, Document Tools for the documentation tools.
        protected ElementToolComponent(
            string name,
            string description,
            string subCategory = null)
            : base(
                name,
                description,
                subCategory ?? GroupNames.DesignTools)
        {
            CapsuleButtonTexts = new List<string> { "Refresh", "Execute", "Create" };
        }

        public List<string> CapsuleButtonTexts { get; set; }

        public override void CreateAttributes()
        {
            m_attributes = new ButtonAttributes(this, 3);
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuid",
                $"{Table.Tool} elements to read (Refresh) or write (Execute).");
            InTexts(
                "ElementID",
                "Element ID to set (Execute, Create): one for all elements or one per element.");
            InIntegers(
                "LayerIndex",
                "Layer attribute index to set (Execute, Create): one for all elements or one per element.");
            InIntegers(
                "DrawOrder",
                "Draw order (drawIndex) to set (Execute, Create): one for all elements or one per element.");
            InTexts(
                "FavoriteName",
                "Favorite the new elements start from (Create): one for all or one per element.");
            AddGeometryInputs();
            foreach (var group in Groups)
            {
                InTexts(
                    GroupParamName(group),
                    $"Settings of the \"{group}\" panel of the {Table.Tool} settings dialog, as a JSON " +
                    "object (Execute, Create): one for all elements or one per element. Only the given " +
                    "keys are written; read-only keys are ignored, so a JSON read by Refresh can be fed back.");
            }

            for (var i = 0; i < inManager.ParamCount; i++)
            {
                SetOptionality(i);
            }
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuid",
                $"The {Table.Tool} elements read, written or created (null where one could not be read or created).");
            OutTexts(
                "ElementID",
                "Element ID of each element.");
            OutIntegers(
                "LayerIndex",
                "Layer attribute index of each element.");
            OutIntegers(
                "DrawOrder",
                "Draw order (drawIndex) of each element.");
            OutTexts(
                "Errors",
                "One message per element (empty when it succeeded) for the last Refresh, Execute or Create.");
            AddGeometryOutputs();
            foreach (var group in Groups)
            {
                OutTexts(
                    GroupParamName(group),
                    $"Settings of the \"{group}\" panel of each element, as a JSON object.");
            }
        }

        protected override void AppendAdditionalComponentMenuItems(
            ToolStripDropDown menu)
        {
            base.AppendAdditionalComponentMenuItems(menu);
            Menu_AppendItem(menu, "Refresh from Archicad", (s, e) => Request(ToolAction.Refresh));
            Menu_AppendItem(menu, "Execute in Archicad", (s, e) => Request(ToolAction.Execute));
            Menu_AppendItem(menu, "Create in Archicad", (s, e) => Request(ToolAction.Create));
            if (!CanUpdateCreatedElements)
            {
                return;
            }

            Menu_AppendItem(
                menu,
                UpdateCreatedElementsText,
                (s, e) =>
                {
                    RecordUndoEvent(UpdateCreatedElementsText);
                    _updateCreatedElements = !_updateCreatedElements;
                },
                true,
                _updateCreatedElements);
            var createdCount = OwnCreatedGuids().Sum(branch => branch.Count(g => g != null));
            Menu_AppendItem(
                menu,
                $"{ForgetCreatedElementsText} ({createdCount})",
                (s, e) =>
                {
                    RecordUndoEvent(ForgetCreatedElementsText);
                    _createdGuids = new List<List<string>>();
                },
                createdCount > 0);
        }

        // The elements this component created; none for a pasted copy,
        // whose instance id differs from the one that created them.
        private List<List<string>> OwnCreatedGuids()
        {
            if (_createdBy != InstanceGuid)
            {
                _createdGuids = new List<List<string>>();
                _createdBy = InstanceGuid;
            }
            return _createdGuids;
        }

        // The elements this component created from the geometry of one solve
        // iteration (one branch of the geometry tree).
        private List<string> OwnCreatedGuids(
            int iteration)
        {
            var created = OwnCreatedGuids();
            while (created.Count <= iteration)
            {
                created.Add(new List<string>());
            }
            return created[iteration];
        }

        public override bool Write(
            GH_IWriter writer)
        {
            if (CanUpdateCreatedElements)
            {
                // One entry per remembered element: its iteration, its
                // geometry item and its guid.
                var created = OwnCreatedGuids();
                writer.SetBoolean(UpdateCreatedElementsKey, _updateCreatedElements);
                var k = 0;
                for (var iteration = 0; iteration < created.Count; iteration++)
                {
                    for (var item = 0; item < created[iteration].Count; item++)
                    {
                        if (created[iteration][item] == null)
                        {
                            continue;
                        }
                        writer.SetInt32(CreatedElementIterationKey, k, iteration);
                        writer.SetInt32(CreatedElementItemKey, k, item);
                        writer.SetString(CreatedElementGuidKey, k, created[iteration][item]);
                        k++;
                    }
                }
                writer.SetInt32(CreatedElementCountKey, k);
            }
            return base.Write(writer);
        }

        public override bool Read(
            GH_IReader reader)
        {
            var result = base.Read(reader);
            _createdGuids = new List<List<string>>();
            _createdBy = InstanceGuid;
            if (CanUpdateCreatedElements)
            {
                if (reader.ItemExists(UpdateCreatedElementsKey))
                {
                    _updateCreatedElements = reader.GetBoolean(UpdateCreatedElementsKey);
                }
                var count = reader.ItemExists(CreatedElementCountKey)
                    ? reader.GetInt32(CreatedElementCountKey)
                    : 0;
                for (var k = 0; k < count; k++)
                {
                    if (!reader.ItemExists(CreatedElementGuidKey, k) ||
                        !reader.ItemExists(CreatedElementIterationKey, k) ||
                        !reader.ItemExists(CreatedElementItemKey, k))
                    {
                        continue;
                    }
                    var guid = reader.GetString(CreatedElementGuidKey, k);
                    var iteration = reader.GetInt32(CreatedElementIterationKey, k);
                    var item = reader.GetInt32(CreatedElementItemKey, k);
                    if (string.IsNullOrEmpty(guid) || iteration < 0 || item < 0)
                    {
                        continue;
                    }
                    var branch = OwnCreatedGuids(iteration);
                    while (branch.Count <= item)
                    {
                        branch.Add(null);
                    }
                    branch[item] = guid;
                }
            }
            return result;
        }

        public void OnCapsuleButtonPressed(
            int buttonIndex)
        {
            // The buttons are drawn from the bottom up.
            Request(
                buttonIndex == 0
                    ? ToolAction.Refresh
                    : buttonIndex == 1
                        ? ToolAction.Execute
                        : ToolAction.Create);
        }

        private void Request(
            ToolAction action)
        {
            switch (action)
            {
                case ToolAction.Refresh:
                    _refreshRequested = true;
                    break;
                case ToolAction.Execute:
                    _executeRequested = true;
                    break;
                case ToolAction.Create:
                    _createRequested = true;
                    break;
            }
            ExpireSolution(true);
        }

        // The requests are taken once per solution, before the first
        // iteration, and cleared there: clearing them after ExpireSolution
        // loses presses when Grasshopper does not solve synchronously, and
        // clearing them in SolveInstance would only serve its first iteration.
        protected override void BeforeSolveInstance()
        {
            base.BeforeSolveInstance();

            _action = _createRequested
                ? ToolAction.Create
                : _executeRequested
                    ? ToolAction.Execute
                    : _refreshRequested || AutoRefresh
                        ? ToolAction.Refresh
                        : ToolAction.None;

            _refreshRequested = false;
            _executeRequested = false;
            _createRequested = false;
        }

        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            switch (_action)
            {
                case ToolAction.Refresh:
                    Refresh(da, ReadElementGuids(da));
                    break;
                case ToolAction.Execute:
                    Execute(da);
                    break;
                case ToolAction.Create:
                    Create(da);
                    break;
                default:
                    AddRuntimeMessage(
                        GH_RuntimeMessageLevel.Remark,
                        "Outdated, waiting for manual refresh");
                    break;
            }
        }

        // ---------------------------------------------------------------
        // Refresh
        // ---------------------------------------------------------------

        private void Refresh(
            IGH_DataAccess da,
            List<string> guids,
            List<string> errors = null)
        {
            errors = errors ?? guids.Select(_ => "").ToList();
            var entries = ReadDetails(guids, errors);

            da.SetDataList(
                ElementGuidIndex,
                guids.Select(
                    (g, i) => g == null || entries[i] == null
                        ? null
                        : new ElementGuidWrapper { ElementId = new ElementGuid { Guid = g } }));
            da.SetDataList(
                ElementIdIndex,
                entries.Select(e => e?["id"]?.ToString()));
            da.SetDataList(
                LayerIndexIndex,
                entries.Select(e => e?["layerIndex"] == null ? (int?)null : (int)e["layerIndex"]));
            da.SetDataList(
                DrawOrderIndex,
                entries.Select(e => e?["drawIndex"] == null ? (int?)null : (int)e["drawIndex"]));

            for (var g = 0; g < Groups.Count; g++)
            {
                var group = Groups[g];
                da.SetDataList(
                    FirstGroupIndex + g,
                    entries.Select(e => e == null ? null : GroupJson(e, group)));
            }

            SetGeometryOutputs(
                da,
                FirstGeometryIndex,
                entries.Select(e => e?["details"] as JObject).ToList());

            da.SetDataList(
                ErrorsOutputIndex,
                errors);
        }

        // One detailsOfElements entry per guid, null (with its error) where
        // the element could not be read or is not of this tool.
        private List<JObject> ReadDetails(
            List<string> guids,
            List<string> errors)
        {
            var entries = guids.Select(_ => (JObject) null).ToList();
            var indices = Enumerable.Range(0, guids.Count).Where(i => guids[i] != null).ToList();
            if (indices.Count == 0)
            {
                return entries;
            }

            var parameters = new JObject
            {
                ["elements"] = new JArray(indices.Select(i => ElementItemObject(guids[i])))
            };
            if (!TryGetCadResponse("GetDetailsOfElements", parameters, ToAddOn, out var response))
            {
                return entries;
            }

            var items = response["detailsOfElements"] as JArray ?? new JArray();
            for (var k = 0; k < indices.Count && k < items.Count; k++)
            {
                var i = indices[k];
                var item = items[k] as JObject;
                if (item == null || item["error"] != null)
                {
                    AddMessage(errors, i, item?["error"]?["message"]?.ToString() ?? "The element could not be read.");
                }
                else if (item["type"]?.ToString() != Table.ElementType)
                {
                    AddMessage(errors, i, $"The element is a {item["type"]}, not a {Table.ElementType}.");
                }
                else
                {
                    entries[i] = item;
                }
            }
            return entries;
        }

        private string GroupJson(
            JObject entry,
            string group)
        {
            var details = entry["details"] as JObject ?? new JObject();
            var json = new JObject();
            foreach (var field in Table.FieldsOf(group))
            {
                if (field.Get == null)
                {
                    continue;
                }
                var value = field.GetFromElement ? entry[field.Get] : details[field.Get];
                if (value != null)
                {
                    json[field.Key(UseLabels)] = value.DeepClone();
                }
            }
            return json.ToString(Formatting.None);
        }

        // ---------------------------------------------------------------
        // Execute
        // ---------------------------------------------------------------

        private void Execute(
            IGH_DataAccess da)
        {
            var guids = ReadElementGuids(da);
            var count = guids.Count;
            if (count == 0)
            {
                AddRuntimeMessage(GH_RuntimeMessageLevel.Warning, "Execute needs the elements in ElementGuid.");
                return;
            }

            if (!TryReadSettingInputs(da, count, out var inputs))
            {
                return;
            }

            var errors = guids.Select(g => g == null ? "Invalid element GUID." : "").ToList();

            // The Modify commands write the tool's fields into whatever element
            // they get, so elements of another type are left out here.
            var targets = KeepElementsOfTool(guids, errors);

            var createOnly = new SortedSet<string>();
            var notes = new SortedSet<string>();
            var modifyItems = new List<KeyValuePair<int, JObject>>();
            var detailItems = new List<KeyValuePair<int, JObject>>();
            for (var i = 0; i < count; i++)
            {
                if (targets[i] == null || !TryMergeSettings(inputs, i, errors, out var settings))
                {
                    continue;
                }
                PrepareSettings(settings, notes);
                AddWriteItems(inputs, i, guids[i], settings, null, createOnly, modifyItems, detailItems);
            }

            SendModify(modifyItems, errors);
            SendDetails(targets, detailItems, errors);
            SetElementIds(targets, inputs.ElementIds, errors);

            AddNotes(notes);
            if (createOnly.Count > 0)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    $"{string.Join(", ", createOnly)} can only be given when creating; not changed on existing elements.");
            }

            var written = modifyItems.Count + detailItems.Count;
            if (written == 0 && inputs.ElementIds.Count == 0)
            {
                AddRuntimeMessage(GH_RuntimeMessageLevel.Remark, "Nothing to write: no setting was given.");
            }

            Refresh(da, guids, errors);
        }

        // Splits the settings of element i (and, when updating an element
        // Create made, its geometry) into its Modify item and its
        // SetDetailsOfElements item. Settings only the Create command takes
        // go to createOnly.
        private void AddWriteItems(
            SettingInputs inputs,
            int i,
            string guid,
            JObject settings,
            JObject geometry,
            ISet<string> createOnly,
            List<KeyValuePair<int, JObject>> modifyItems,
            List<KeyValuePair<int, JObject>> detailItems)
        {
            var modify = new JObject();
            var details = new JObject();
            foreach (var setting in settings)
            {
                // The home story goes with the Modify command when it takes
                // it (it pins the story a Z there is measured from), else to
                // SetDetailsOfElements.
                if (setting.Key == "floorIndex" && !ModifyKeys.Contains(setting.Key))
                {
                    details["floorIndex"] = setting.Value;
                }
                else if (ModifyKeys.Contains(setting.Key))
                {
                    modify[setting.Key] = setting.Value;
                }
                else if (TypeSpecificDetailsKeys.Contains(setting.Key))
                {
                    if (!(details["typeSpecificDetails"] is JObject typeSpecific))
                    {
                        typeSpecific = new JObject();
                        details["typeSpecificDetails"] = typeSpecific;
                    }
                    typeSpecific[setting.Key] = setting.Value;
                }
                else
                {
                    // E.g. the bottom elevation of a JSON read by Refresh.
                    createOnly.Add(KeyOf(setting.Key));
                }
            }

            // As on Create, the geometry inputs win over geometry in the JSON.
            if (geometry != null)
            {
                foreach (var property in geometry.Properties())
                {
                    if (ModifyKeys.Contains(property.Name))
                    {
                        modify[property.Name] = property.Value;
                    }
                    else
                    {
                        createOnly.Add(KeyOf(property.Name));
                    }
                }
            }
            AddPlainDetails(inputs, i, details);

            if (modify.Count > 0)
            {
                modify["elementId"] = ElementIdObject(guid);
                modifyItems.Add(new KeyValuePair<int, JObject>(i, modify));
            }
            if (details.Count > 0)
            {
                detailItems.Add(new KeyValuePair<int, JObject>(i, details));
            }
        }

        // ---------------------------------------------------------------
        // Create
        // ---------------------------------------------------------------

        private void Create(
            IGH_DataAccess da)
        {
            if (!TryGetGeometry(da, FirstGeometryIndex, out var geometry))
            {
                return;
            }
            var count = geometry.Count;
            if (count == 0)
            {
                this.AddError("Create needs the geometry inputs.");
                return;
            }

            if (!TryReadSettingInputs(da, count, out var inputs))
            {
                return;
            }

            var errors = Enumerable.Repeat("", count).ToList();
            var notes = new SortedSet<string>();

            // Item i of the geometry updates the element an earlier Create
            // made from item i, when it still exists; a deleted one is an
            // error, never made again (#521).
            var update = CanUpdateCreatedElements && _updateCreatedElements;
            var created = CanUpdateCreatedElements ? OwnCreatedGuids(da.Iteration) : new List<string>();
            var remembered = Enumerable.Range(0, count)
                .Select(i => update && i < created.Count ? created[i] : null)
                .ToList();
            var existing = KeepElementsOfTool(remembered, errors);

            var guids = Enumerable.Repeat((string) null, count).ToList();
            var createOnly = new SortedSet<string>();
            var modifyItems = new List<KeyValuePair<int, JObject>>();
            var detailItems = new List<KeyValuePair<int, JObject>>();
            var updated = 0;
            var createIndices = new List<int>();
            var createItems = new JArray();
            var postModify = new Dictionary<int, JObject>();
            for (var i = 0; i < count; i++)
            {
                if (remembered[i] != null && existing[i] == null)
                {
                    AddMessage(
                        errors,
                        i,
                        "This component created it on an earlier Create; it was not created again. " +
                        $"\"{ForgetCreatedElementsText}\" in the component menu makes Create make new elements.");
                    continue;
                }

                if (!TryMergeSettings(inputs, i, errors, out var settings))
                {
                    continue;
                }
                PrepareSettings(settings, notes);

                if (existing[i] != null)
                {
                    guids[i] = existing[i];
                    AddWriteItems(inputs, i, guids[i], settings, geometry[i], createOnly, modifyItems, detailItems);
                    if (!string.IsNullOrEmpty(ItemOf(inputs.FavoriteNames, i)))
                    {
                        createOnly.Add("FavoriteName");
                    }
                    updated++;
                    continue;
                }
                PrepareCreateSettings(settings, notes);

                var item = new JObject();
                var rest = new JObject();
                foreach (var setting in settings)
                {
                    if (CreateKeys.Contains(setting.Key))
                    {
                        item[setting.Key] = setting.Value;
                    }
                    else if (ModifyKeys.Contains(setting.Key))
                    {
                        // Written right after the element is created.
                        rest[setting.Key] = setting.Value;
                    }
                    else
                    {
                        AddMessage(errors, i, $"\"{KeyOf(setting.Key)}\" cannot be set; it was ignored.");
                    }
                }

                // The geometry inputs always win over geometry in the JSON,
                // so a JSON read from another element never moves the new one.
                foreach (var property in geometry[i].Properties())
                {
                    item[property.Name] = property.Value;
                }
                var favorite = ItemOf(inputs.FavoriteNames, i);
                if (!string.IsNullOrEmpty(favorite))
                {
                    item["favoriteName"] = favorite;
                }
                PrepareCreateItem(item);

                createIndices.Add(i);
                createItems.Add(item);
                if (rest.Count > 0)
                {
                    postModify[i] = rest;
                }
            }

            if (createItems.Count > 0 &&
                TryGetCadResponse(CreateCommand, new JObject { [CreateArrayKey] = createItems }, ToAddOn, out var response))
            {
                var results = response["elements"] as JArray ?? new JArray();
                for (var k = 0; k < createIndices.Count; k++)
                {
                    var i = createIndices[k];
                    var result = k < results.Count ? results[k] : null;
                    var guid = result?["elementId"]?["guid"]?.ToString();
                    if (guid != null)
                    {
                        guids[i] = guid;
                    }
                    else
                    {
                        AddMessage(errors, i, result?["error"]?["message"]?.ToString() ?? "The element was not created.");
                    }
                }
            }

            foreach (var i in createIndices)
            {
                if (guids[i] == null)
                {
                    continue;
                }
                if (postModify.TryGetValue(i, out var rest))
                {
                    rest["elementId"] = ElementIdObject(guids[i]);
                    modifyItems.Add(new KeyValuePair<int, JObject>(i, rest));
                }
                var details = new JObject();
                AddPlainDetails(inputs, i, details);
                if (details.Count > 0)
                {
                    detailItems.Add(new KeyValuePair<int, JObject>(i, details));
                }
            }
            SendModify(modifyItems, errors);
            SendDetails(guids, detailItems, errors);
            SetElementIds(guids, inputs.ElementIds, errors);

            if (CanUpdateCreatedElements)
            {
                RememberCreated(da.Iteration, guids, createIndices, update);
            }

            AddNotes(notes);
            if (updated > 0)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    $"Updated {updated} element(s) this component created before instead of creating new ones " +
                    $"(\"{UpdateCreatedElementsText}\" in the component menu).");
            }
            if (createOnly.Count > 0)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    $"{string.Join(", ", createOnly)} can only be given when creating; not changed on the elements created before.");
            }
            var unused = update ? created.Skip(count).Count(g => g != null) : 0;
            if (unused > 0)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    $"{unused} element(s) created before have no geometry item now; they were left unchanged.");
            }
            Refresh(da, guids, errors);
        }

        // Stores the elements made by this Create at their geometry item's
        // position. Without updating, this Create's elements replace the
        // earlier ones, so switching updating on later updates these.
        private void RememberCreated(
            int iteration,
            List<string> guids,
            List<int> createIndices,
            bool update)
        {
            if (!update && iteration == 0)
            {
                // A new set of elements: forget every branch of the earlier one.
                OwnCreatedGuids().Clear();
            }
            var created = OwnCreatedGuids(iteration);
            if (!update)
            {
                created.Clear();
            }
            foreach (var i in createIndices)
            {
                if (guids[i] == null)
                {
                    continue;
                }
                while (created.Count <= i)
                {
                    created.Add(null);
                }
                created[i] = guids[i];
            }
        }

        // ---------------------------------------------------------------
        // Inputs
        // ---------------------------------------------------------------

        private class SettingInputs
        {
            public List<string> ElementIds = new List<string>();
            public List<int> LayerIndices = new List<int>();
            public List<int> DrawOrders = new List<int>();
            public List<List<string>> GroupJsons = new List<List<string>>();
            public List<string> FavoriteNames = new List<string>();
        }

        private List<string> ReadElementGuids(
            IGH_DataAccess da)
        {
            var wrappers = new List<GH_ObjectWrapper>();
            da.GetDataList(ElementGuidIndex, wrappers);
            return wrappers
                .Select(w => ElementGuid.CreateFromWrapper(w)?.Guid)
                .ToList();
        }

        // Every list input may hold nothing, one item (used for every
        // element) or one item per element.
        private bool TryReadSettingInputs(
            IGH_DataAccess da,
            int count,
            out SettingInputs inputs)
        {
            inputs = new SettingInputs();
            da.GetDataList(ElementIdIndex, inputs.ElementIds);
            da.GetDataList(LayerIndexIndex, inputs.LayerIndices);
            da.GetDataList(DrawOrderIndex, inputs.DrawOrders);
            da.GetDataList(FavoriteNameIndex, inputs.FavoriteNames);
            for (var g = 0; g < Groups.Count; g++)
            {
                var jsons = new List<string>();
                da.GetDataList(FirstGroupIndex + g, jsons);
                inputs.GroupJsons.Add(jsons);
            }

            var lists = new List<KeyValuePair<string, int>>
            {
                new KeyValuePair<string, int>("ElementID", inputs.ElementIds.Count),
                new KeyValuePair<string, int>("LayerIndex", inputs.LayerIndices.Count),
                new KeyValuePair<string, int>("DrawOrder", inputs.DrawOrders.Count),
                new KeyValuePair<string, int>("FavoriteName", inputs.FavoriteNames.Count)
            };
            for (var g = 0; g < Groups.Count; g++)
            {
                lists.Add(new KeyValuePair<string, int>(GroupParamName(Groups[g]), inputs.GroupJsons[g].Count));
            }

            var ok = true;
            foreach (var list in lists)
            {
                if (list.Value > 1 && list.Value != count)
                {
                    this.AddError(
                        $"{list.Key} has {list.Value} items: give none, one for all {count} elements, or one per element.");
                    ok = false;
                }
            }
            return ok;
        }

        // The settings of all JSON inputs for element i, as Add-On keys.
        // Read-only keys are skipped (a Refresh JSON can be fed back), an
        // unknown key or invalid JSON is reported in the element's message.
        private bool TryMergeSettings(
            SettingInputs inputs,
            int i,
            List<string> errors,
            out JObject settings)
        {
            settings = new JObject();
            for (var g = 0; g < Groups.Count; g++)
            {
                var text = ItemOf(inputs.GroupJsons[g], i);
                if (string.IsNullOrWhiteSpace(text))
                {
                    continue;
                }

                JObject json;
                try
                {
                    json = JObject.Parse(text);
                }
                catch (JsonException ex)
                {
                    AddMessage(errors, i, $"{GroupParamName(Groups[g])} is not a valid JSON object: {ex.Message}");
                    return false;
                }

                foreach (var property in json.Properties())
                {
                    var field = Table.Find(property.Name);
                    if (field == null)
                    {
                        AddMessage(errors, i, $"Unknown setting \"{property.Name}\" in {GroupParamName(Groups[g])}; it was ignored.");
                        continue;
                    }
                    if (field.Set == null || property.Value.Type == JTokenType.Null)
                    {
                        continue;
                    }
                    settings[field.Set] = property.Value.DeepClone();
                }
            }
            return true;
        }

        private static void AddPlainDetails(
            SettingInputs inputs,
            int i,
            JObject details)
        {
            if (inputs.LayerIndices.Count > 0)
            {
                details["layerIndex"] = ItemOf(inputs.LayerIndices, i);
            }
            if (inputs.DrawOrders.Count > 0)
            {
                details["drawIndex"] = ItemOf(inputs.DrawOrders, i);
            }
        }

        // ---------------------------------------------------------------
        // Writing
        // ---------------------------------------------------------------

        private void SendModify(
            List<KeyValuePair<int, JObject>> items,
            List<string> errors)
        {
            if (items.Count == 0)
            {
                return;
            }

            var parameters = new JObject { [ModifyArrayKey] = new JArray(items.Select(p => ModifyItemOf(p.Value))) };
            if (TryGetCadResponse(ModifyCommand, parameters, ToAddOn, out var response))
            {
                AddExecutionResults(response, items.Select(p => p.Key).ToList(), errors);
            }
        }

        // { elementId, settings... } as the Modify command takes it: as is,
        // or with the settings under ModifyItemSettingsKey.
        private JObject ModifyItemOf(
            JObject item)
        {
            if (ModifyItemSettingsKey == null)
            {
                return item;
            }

            var settings = new JObject(item.Properties().Where(p => p.Name != "elementId"));
            return new JObject
            {
                ["elementId"] = item["elementId"],
                [ModifyItemSettingsKey] = settings
            };
        }

        private void SendDetails(
            List<string> guids,
            List<KeyValuePair<int, JObject>> items,
            List<string> errors)
        {
            if (items.Count == 0)
            {
                return;
            }

            var parameters = new JObject
            {
                ["elementsWithDetails"] = new JArray(items.Select(
                    p => new JObject
                    {
                        ["elementId"] = ElementIdObject(guids[p.Key]),
                        ["details"] = p.Value
                    }))
            };
            if (TryGetCadResponse("SetDetailsOfElements", parameters, ToAddOn, out var response))
            {
                AddExecutionResults(response, items.Select(p => p.Key).ToList(), errors);
            }
        }

        // The Element ID is the built-in General_ElementID property; no
        // details command writes it.
        private void SetElementIds(
            List<string> guids,
            List<string> elementIds,
            List<string> errors)
        {
            var indices = Enumerable.Range(0, guids.Count).Where(i => guids[i] != null).ToList();
            if (elementIds.Count == 0 || indices.Count == 0)
            {
                return;
            }

            var idsResponse = ToArchicad(
                "GetPropertyIds",
                new JObject
                {
                    ["properties"] = new JArray
                    {
                        new JObject { ["type"] = "BuiltIn", ["nonLocalizedName"] = ElementIdPropertyName }
                    }
                });
            var propertyGuid = idsResponse.Succeeded
                ? idsResponse.Result?["properties"]?[0]?["propertyId"]?["guid"]?.ToString()
                : null;
            if (propertyGuid == null)
            {
                AddRuntimeMessage(GH_RuntimeMessageLevel.Warning, "The Element ID property was not found; ElementID was not set.");
                return;
            }

            var values = new JArray(indices.Select(
                i => new JObject
                {
                    ["elementId"] = ElementIdObject(guids[i]),
                    ["propertyId"] = new JObject { ["guid"] = propertyGuid },
                    ["propertyValue"] = new JObject { ["value"] = ItemOf(elementIds, i) ?? "" }
                }));
            if (TryGetCadResponse(
                    "SetPropertyValuesOfElements",
                    new JObject { ["elementPropertyValues"] = values },
                    ToAddOn,
                    out var response))
            {
                AddExecutionResults(response, indices, errors);
            }
        }

        private static void AddExecutionResults(
            JObject response,
            List<int> indices,
            List<string> errors)
        {
            var messages = ExecutionResultsResponse.ErrorMessages(response);
            for (var k = 0; k < indices.Count && k < messages.Count; k++)
            {
                AddMessage(errors, indices[k], messages[k]);
            }
        }

        // The guids of the elements that are of this tool (null for the
        // others, with a message), read with GetDetailsOfElements' type field.
        private List<string> KeepElementsOfTool(
            List<string> guids,
            List<string> errors)
        {
            var targets = guids.Select(_ => (string) null).ToList();
            var indices = Enumerable.Range(0, guids.Count).Where(i => guids[i] != null).ToList();
            if (indices.Count == 0)
            {
                return targets;
            }

            var parameters = new JObject
            {
                ["elements"] = new JArray(indices.Select(i => ElementItemObject(guids[i]))),
                ["fields"] = new JArray("type")
            };
            if (!TryGetCadResponse("GetDetailsOfElements", parameters, ToAddOn, out var response))
            {
                return targets;
            }

            var items = response["detailsOfElements"] as JArray ?? new JArray();
            for (var k = 0; k < indices.Count; k++)
            {
                var i = indices[k];
                var item = k < items.Count ? items[k] as JObject : null;
                var type = item?["type"]?.ToString();
                if (type == Table.ElementType)
                {
                    targets[i] = guids[i];
                }
                else if (item == null || item["error"] != null || type == null)
                {
                    AddMessage(errors, i, item?["error"]?["message"]?.ToString() ?? "The element could not be read; it was not changed.");
                }
                else
                {
                    AddMessage(errors, i, $"The element is a {type}, not a {Table.ElementType}; it was not changed.");
                }
            }
            return targets;
        }

        private void AddNotes(
            ISet<string> notes)
        {
            foreach (var note in notes)
            {
                AddRuntimeMessage(GH_RuntimeMessageLevel.Remark, note);
            }
        }

        // ---------------------------------------------------------------
        // Helpers
        // ---------------------------------------------------------------

        private string KeyOf(
            string apiName)
        {
            return Table.Find(apiName)?.Key(UseLabels) ?? apiName;
        }

        protected static JObject ElementIdObject(
            string guid)
        {
            return new JObject { ["guid"] = guid };
        }

        // An item of an "elements" array (ElementIdArrayItem):
        // { "elementId": { "guid": ... } }.
        protected static JObject ElementItemObject(
            string guid)
        {
            return new JObject { ["elementId"] = ElementIdObject(guid) };
        }

        private static T ItemOf<T>(
            List<T> list,
            int i)
        {
            return list.Count == 0 ? default : list.Count == 1 ? list[0] : list[i];
        }

        private static void AddMessage(
            List<string> errors,
            int i,
            string message)
        {
            if (string.IsNullOrEmpty(message))
            {
                return;
            }
            errors[i] = string.IsNullOrEmpty(errors[i]) ? message : errors[i] + " " + message;
        }
    }
}
