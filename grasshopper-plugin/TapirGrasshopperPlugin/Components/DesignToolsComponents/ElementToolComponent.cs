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
    //   Create   makes new elements from the geometry inputs and the same JSON.
    // Writing and creating only happen on a button press, never through
    // AutoExecute. Deleting elements is left to the DeleteElements component
    // on purpose. See CLAUDE.md, "Archicad tool components".
    public abstract class ElementToolComponent : Component, IButtonComponent
    {
        private const int ElementGuidIndex = 0;
        private const int ElementIdIndex = 1;
        private const int LayerIndexIndex = 2;
        private const int DrawOrderIndex = 3;
        private const int FirstGroupIndex = 4;

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

        // The tool's table name (ToolSettings/<ToolName>.json).
        protected abstract string ToolName { get; }

        // True: JSON keys are the dialog labels; false: the Add-On's names.
        protected abstract bool UseLabels { get; }

        protected abstract string CreateCommand { get; }
        protected abstract string CreateArrayKey { get; }
        protected abstract string ModifyCommand { get; }
        protected abstract string ModifyArrayKey { get; }

        // The item keys the Create and the Modify command accept (their input
        // schemas), so a setting goes to the command that can write it.
        protected abstract ISet<string> CreateKeys { get; }
        protected abstract ISet<string> ModifyKeys { get; }

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

        private List<string> Groups => Table.Groups;

        private int FavoriteNameIndex => FirstGroupIndex + Groups.Count;
        private int FirstGeometryInputIndex => FavoriteNameIndex + 1;
        private int FirstGeometryOutputIndex => FirstGroupIndex + Groups.Count;
        private int ErrorsOutputIndex => FirstGeometryOutputIndex + GeometryInputCount;

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

        protected ElementToolComponent(
            string name,
            string description)
            : base(
                name,
                description,
                GroupNames.DesignTools)
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
            foreach (var group in Groups)
            {
                InTexts(
                    GroupParamName(group),
                    $"Settings of the \"{group}\" panel of the {Table.Tool} settings dialog, as a JSON " +
                    "object (Execute, Create): one for all elements or one per element. Only the given " +
                    "keys are written; read-only keys are ignored, so a JSON read by Refresh can be fed back.");
            }
            InTexts(
                "FavoriteName",
                "Favorite the new elements start from (Create): one for all or one per element.");
            AddGeometryInputs();

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
            foreach (var group in Groups)
            {
                OutTexts(
                    GroupParamName(group),
                    $"Settings of the \"{group}\" panel of each element, as a JSON object.");
            }
            AddGeometryOutputs();
            OutTexts(
                "Errors",
                "One message per element (empty when it succeeded) for the last Refresh, Execute or Create.");
        }

        protected override void AppendAdditionalComponentMenuItems(
            ToolStripDropDown menu)
        {
            base.AppendAdditionalComponentMenuItems(menu);
            Menu_AppendItem(menu, "Refresh from Archicad", (s, e) => Request(ToolAction.Refresh));
            Menu_AppendItem(menu, "Execute in Archicad", (s, e) => Request(ToolAction.Execute));
            Menu_AppendItem(menu, "Create in Archicad", (s, e) => Request(ToolAction.Create));
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
                FirstGeometryOutputIndex,
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
                ["elements"] = new JArray(indices.Select(i => ElementIdObject(guids[i])))
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
            var createOnly = new SortedSet<string>();
            var modifyItems = new List<KeyValuePair<int, JObject>>();
            var detailItems = new List<KeyValuePair<int, JObject>>();
            for (var i = 0; i < count; i++)
            {
                if (guids[i] == null || !TryMergeSettings(inputs, i, errors, out var settings))
                {
                    continue;
                }

                var modify = new JObject();
                var details = new JObject();
                foreach (var setting in settings)
                {
                    if (setting.Key == "floorIndex")
                    {
                        details["floorIndex"] = setting.Value;
                    }
                    else if (ModifyKeys.Contains(setting.Key))
                    {
                        modify[setting.Key] = setting.Value;
                    }
                    else
                    {
                        // E.g. the bottom elevation of a JSON read by Refresh.
                        createOnly.Add(KeyOf(setting.Key));
                    }
                }
                AddPlainDetails(inputs, i, details);

                if (modify.Count > 0)
                {
                    modify["elementId"] = ElementIdObject(guids[i]);
                    modifyItems.Add(new KeyValuePair<int, JObject>(i, modify));
                }
                if (details.Count > 0)
                {
                    detailItems.Add(new KeyValuePair<int, JObject>(i, details));
                }
            }

            SendModify(modifyItems, errors);
            SendDetails(guids, detailItems, errors);
            SetElementIds(guids, inputs.ElementIds, errors);

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

        // ---------------------------------------------------------------
        // Create
        // ---------------------------------------------------------------

        private void Create(
            IGH_DataAccess da)
        {
            if (!TryGetGeometry(da, FirstGeometryInputIndex, out var geometry))
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
            var createIndices = new List<int>();
            var createItems = new JArray();
            var postModify = new Dictionary<int, JObject>();
            for (var i = 0; i < count; i++)
            {
                if (!TryMergeSettings(inputs, i, errors, out var settings))
                {
                    continue;
                }

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

            var guids = Enumerable.Repeat((string) null, count).ToList();
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

            var modifyItems = new List<KeyValuePair<int, JObject>>();
            var detailItems = new List<KeyValuePair<int, JObject>>();
            for (var i = 0; i < count; i++)
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

            Refresh(da, guids, errors);
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

            var parameters = new JObject { [ModifyArrayKey] = new JArray(items.Select(p => p.Value)) };
            if (TryGetCadResponse(ModifyCommand, parameters, ToAddOn, out var response))
            {
                AddExecutionResults(response, items.Select(p => p.Key).ToList(), errors);
            }
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
