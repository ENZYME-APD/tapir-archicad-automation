using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetAttributesFromProjectComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAttributesFromProject";

        public GetAttributesFromProjectComponent()
            : base(
                "GetAttributesFromProject",
                "Reads the attributes of the given types from another project or template file without opening it, " +
                "for example to compare them with the attributes of the open project. Nothing is imported into the " +
                "open project. The names are the legacy 8-bit names, so non-ASCII characters may not come through. " +
                "Archicad does not read Layer, Surface, BuildingMaterial and MEPSystem attributes from a file that " +
                "is not open: these types get an error item instead of an empty list. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ProjectFilePath",
                "The absolute path of the project or template file (for example .pln or .tpl) to read the attributes from.");

            InTexts(
                "AttributeTypes",
                "The attribute types to read: Layer, Line, Fill, Composite, Surface, LayerCombination, ZoneCategory, " +
                "Profile, PenTable, MEPSystem, OperationProfile or BuildingMaterial.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "AttributeTypes",
                "Attribute type of each branch of the other outputs, in the order of the input AttributeTypes.");

            OutGenericTree(
                "AttributeGuids",
                "Identifiers of the attributes in the file (one branch per attribute type).");

            OutIntegerTree(
                "Indices",
                "Indices of the attributes in the file (one branch per attribute type).");

            OutTextTree(
                "Names",
                "Names of the attributes in the file (one branch per attribute type).");

            outManager.AddTimeParameter(
                "ModificationTimes",
                "ModificationTimes",
                "Last modification time of each attribute, in local time (one branch per attribute type).",
                GH_ParamAccess.tree);

            OutErrorMessages(
                "Error message of each attribute type (empty when it was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string projectFilePath))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> attributeTypes))
            {
                return;
            }

            var parameters = new JObject
            {
                ["projectFilePath"] = projectFilePath,
                ["attributeTypes"] = new JArray(attributeTypes)
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var items = JsonOutputHelp.Items(
                response,
                "attributesByType");
            var types = new List<object>();
            var guids = new DataTree<object>();
            var indices = new DataTree<object>();
            var names = new DataTree<object>();
            var modificationTimes = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                guids.EnsurePath(path);
                indices.EnsurePath(path);
                names.EnsurePath(path);
                modificationTimes.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    types.Add(i < attributeTypes.Count ? attributeTypes[i] : null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                types.Add(JsonOutputHelp.Scalar(item, "attributeType"));
                errors.Add("");

                if (!(item["attributes"] is JArray attributes))
                {
                    continue;
                }

                foreach (var attribute in attributes)
                {
                    guids.Add(
                        AttributeGuidOf(attribute),
                        path);
                    indices.Add(
                        JsonOutputHelp.Scalar(attribute, "index"),
                        path);
                    names.Add(
                        JsonOutputHelp.Scalar(attribute, "name"),
                        path);
                    var modificationTime = attribute["modificationTime"];
                    modificationTimes.Add(
                        modificationTime == null || modificationTime.Type == JTokenType.Null
                            ? (object)null
                            : DateTimeOffset.FromUnixTimeSeconds(
                                modificationTime.Value<long>()).LocalDateTime,
                        path);
                }
            }

            da.SetDataList(0, types);
            da.SetDataTree(1, guids);
            da.SetDataTree(2, indices);
            da.SetDataTree(3, names);
            da.SetDataTree(4, modificationTimes);
            da.SetDataList(5, errors);
        }

        // House-style guid wrapper of the attributeId of an item; null when absent.
        private static AttributeGuidWrapper AttributeGuidOf(
            JToken item)
        {
            var guid = JsonOutputHelp.WrappedGuidOf(
                item,
                "attributeId");
            if (guid == null)
            {
                return null;
            }
            return new AttributeGuidWrapper
            {
                AttributeId = new AttributeGuidObject { Guid = guid }
            };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAttributesFromProject;

        public override Guid ComponentGuid =>
            new Guid("adc9e85c-a958-4613-af64-49e5ceefad7b");
    }
}
