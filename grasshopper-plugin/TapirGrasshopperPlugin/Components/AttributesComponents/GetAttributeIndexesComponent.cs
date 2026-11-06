using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetAttributeIndexesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAttributeIndexes";

        public GetAttributeIndexesComponent()
            : base(
                "GetAttributeIndexes",
                "Reports for each given attribute type how many attributes there are, the first valid and the " +
                "highest index in use, and the free indexes between them (for example those of deleted " +
                "attributes). Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "AttributeTypes",
                "The attribute types to report the indexes of, e.g. Layer, Line, Fill, Composite, Surface, " +
                "LayerCombination, ZoneCategory, Profile, PenTable, MEPSystem, OperationProfile or " +
                "BuildingMaterial.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "AttributeTypes",
                "The attribute type of each item.");

            OutIntegers(
                "Counts",
                "The number of attributes of each type.");

            OutIntegers(
                "FirstIndices",
                "The index of the first valid attribute of each type (empty when the type has no attribute).");

            OutIntegers(
                "LastIndices",
                "The highest index in use of each type (empty when the type has no attribute).");

            OutIntegerTree(
                "FreeIndices",
                "The indexes between the first and the highest index that no attribute uses, in ascending " +
                "order (one branch per attribute type).");

            OutErrorMessages(
                "Error message of each attribute type (empty when it was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> attributeTypes) ||
                attributeTypes.Count == 0)
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["attributeTypes"] = new JArray(attributeTypes) },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var types = new List<object>();
            var counts = new List<object>();
            var firstIndices = new List<object>();
            var lastIndices = new List<object>();
            var freeIndices = new DataTree<int>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "attributeIndexes");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                freeIndices.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    types.Add(null);
                    counts.Add(null);
                    firstIndices.Add(null);
                    lastIndices.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                types.Add(JsonOutputHelp.Scalar(item, "attributeType"));
                counts.Add(JsonOutputHelp.Scalar(item, "count"));
                firstIndices.Add(JsonOutputHelp.Scalar(item, "firstIndex"));
                lastIndices.Add(JsonOutputHelp.Scalar(item, "lastIndex"));
                if (item["freeIndexes"] is JArray free)
                {
                    foreach (var index in free)
                    {
                        freeIndices.Add(
                            (int)index,
                            path);
                    }
                }
                errors.Add("");
            }

            da.SetDataList(0, types);
            da.SetDataList(1, counts);
            da.SetDataList(2, firstIndices);
            da.SetDataList(3, lastIndices);
            da.SetDataTree(4, freeIndices);
            da.SetDataList(5, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AttributesByType;

        public override Guid ComponentGuid =>
            new Guid("bc402338-33d0-443c-830e-33057dc17bb5");
    }
}
