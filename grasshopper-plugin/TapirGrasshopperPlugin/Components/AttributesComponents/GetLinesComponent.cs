using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetLinesComponent : AttributeStructuredGetterComponent
    {
        public override string CommandName => "GetLines";

        public GetLinesComponent()
            : base(
                "GetLines",
                "Get the details of the given Line attributes.")
        {
        }

        protected override string ResponseArrayKey => "lines";

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "Identifier of each line attribute.");

            OutIntegers(
                "Indices",
                "Index of each line attribute.");

            OutTexts(
                "Names",
                "Name of each line attribute.");

            OutBooleans(
                "ScaleWithPlan",
                "True if the line is scale independent.");

            OutNumberList(
                "DefineScales",
                "Scale the line is defined at.");

            OutTexts(
                "LineTypes",
                "Type of each line: Solid, Dashed or Symbol.");

            OutNumberList(
                "Periods",
                "Length of the line pattern period.");

            OutNumberList(
                "Heights",
                "Height of the line pattern.");

            OutNumbers(
                "DashLengths",
                "Dash lengths of each dashed line (one branch per attribute).");

            OutNumbers(
                "GapLengths",
                "Gap lengths of each dashed line (one branch per attribute).");

            OutTextTree(
                "ItemTypes",
                "Type of each symbol line item (one branch per attribute).");

            OutNumbers(
                "ItemCenterOffsets",
                "Center offset of each symbol line item (one branch per attribute).");

            OutNumbers(
                "ItemLengths",
                "Length of each symbol line item (one branch per attribute).");

            OutPointTree(
                "ItemBegPoints",
                "Begin position of each symbol line item (one branch per attribute).");

            OutPointTree(
                "ItemEndPoints",
                "End position of each symbol line item (one branch per attribute).");

            OutNumbers(
                "ItemRadii",
                "Radius of each symbol line item (one branch per attribute).");

            OutNumbers(
                "ItemBeginAngles",
                "Begin angle of each symbol line item (one branch per attribute).");

            OutNumbers(
                "ItemEndAngles",
                "End angle of each symbol line item (one branch per attribute).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried attribute (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var guids = new List<object>();
            var indices = new List<object>();
            var names = new List<object>();
            var scaleWithPlan = new List<object>();
            var defineScales = new List<object>();
            var lineTypes = new List<object>();
            var periods = new List<object>();
            var heights = new List<object>();
            var dashLengths = new DataTree<object>();
            var gapLengths = new DataTree<object>();
            var itemTypes = new DataTree<object>();
            var itemCenterOffsets = new DataTree<object>();
            var itemLengths = new DataTree<object>();
            var itemBegPoints = new DataTree<object>();
            var itemEndPoints = new DataTree<object>();
            var itemRadii = new DataTree<object>();
            var itemBeginAngles = new DataTree<object>();
            var itemEndAngles = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                dashLengths.EnsurePath(path);
                gapLengths.EnsurePath(path);
                itemTypes.EnsurePath(path);
                itemCenterOffsets.EnsurePath(path);
                itemLengths.EnsurePath(path);
                itemBegPoints.EnsurePath(path);
                itemEndPoints.EnsurePath(path);
                itemRadii.EnsurePath(path);
                itemBeginAngles.EnsurePath(path);
                itemEndAngles.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    guids.Add(null);
                    indices.Add(null);
                    names.Add(null);
                    scaleWithPlan.Add(null);
                    defineScales.Add(null);
                    lineTypes.Add(null);
                    periods.Add(null);
                    heights.Add(null);
                    continue;
                }

                errors.Add("");
                guids.Add(AttributeIdOf(item));
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                names.Add(JsonOutputHelp.Scalar(item, "name"));
                scaleWithPlan.Add(JsonOutputHelp.Scalar(item, "scaleWithPlan"));
                defineScales.Add(JsonOutputHelp.Scalar(item, "defineScale"));
                lineTypes.Add(JsonOutputHelp.Scalar(item, "lineType"));
                periods.Add(JsonOutputHelp.Scalar(item, "period"));
                heights.Add(JsonOutputHelp.Scalar(item, "height"));

                if (item["dashItems"] is JArray dashItems)
                {
                    foreach (var dashItem in dashItems)
                    {
                        dashLengths.Add(
                            JsonOutputHelp.Scalar(dashItem, "dash"),
                            path);
                        gapLengths.Add(
                            JsonOutputHelp.Scalar(dashItem, "gap"),
                            path);
                    }
                }

                if (item["lineItems"] is JArray lineItems)
                {
                    foreach (var lineItem in lineItems)
                    {
                        itemTypes.Add(
                            JsonOutputHelp.Scalar(lineItem, "itemType"),
                            path);
                        itemCenterOffsets.Add(
                            JsonOutputHelp.Scalar(lineItem, "centerOffset"),
                            path);
                        itemLengths.Add(
                            JsonOutputHelp.Scalar(lineItem, "length"),
                            path);
                        itemBegPoints.Add(
                            JsonOutputHelp.Point(lineItem["begPos"]),
                            path);
                        itemEndPoints.Add(
                            JsonOutputHelp.Point(lineItem["endPos"]),
                            path);
                        itemRadii.Add(
                            JsonOutputHelp.Scalar(lineItem, "radius"),
                            path);
                        itemBeginAngles.Add(
                            JsonOutputHelp.Scalar(lineItem, "beginAngle"),
                            path);
                        itemEndAngles.Add(
                            JsonOutputHelp.Scalar(lineItem, "endAngle"),
                            path);
                    }
                }
            }

            da.SetDataList(0, guids);
            da.SetDataList(1, indices);
            da.SetDataList(2, names);
            da.SetDataList(3, scaleWithPlan);
            da.SetDataList(4, defineScales);
            da.SetDataList(5, lineTypes);
            da.SetDataList(6, periods);
            da.SetDataList(7, heights);
            da.SetDataTree(8, dashLengths);
            da.SetDataTree(9, gapLengths);
            da.SetDataTree(10, itemTypes);
            da.SetDataTree(11, itemCenterOffsets);
            da.SetDataTree(12, itemLengths);
            da.SetDataTree(13, itemBegPoints);
            da.SetDataTree(14, itemEndPoints);
            da.SetDataTree(15, itemRadii);
            da.SetDataTree(16, itemBeginAngles);
            da.SetDataTree(17, itemEndAngles);
            da.SetDataList(18, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLines;

        public override Guid ComponentGuid =>
            new Guid("d27a0625-35bb-407b-82e0-6a8017c589e2");
    }
}
