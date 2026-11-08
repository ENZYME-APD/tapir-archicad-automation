using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SplitElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SplitElements";

        public SplitElementsComponent()
            : base(
                "SplitElements",
                "Split each given element in two along a line (the infinite line through its begin and end " +
                "point, like Archicad's Split command). The original element keeps the part from its begin " +
                "point to the line, a copy of it with every setting gets the rest. Supports straight walls " +
                "without doors, windows or openings, straight unslanted single segment beams without holes or " +
                "openings, and lines. One undo step.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to split.");

            inManager.AddLineParameter(
                "SplitLine",
                "SplitLine",
                "The split line on the floor plan: the infinite line through its start and end point (only X " +
                "and Y are used).",
                GH_ParamAccess.item);
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ElementGuids",
                "The two parts of each split element: first the original element, then the new one (one " +
                "branch per input element, empty when it was not split).");

            OutBooleans(
                "NotCut",
                "True for an element the split line does not cross (or only touches at one of its ends); it " +
                "is left as it was.");

            OutErrorMessages(
                "Error message of each element (empty when it was split or not cut), for example for an " +
                "unsupported element type.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            var splitLine = Line.Unset;
            if (!da.GetData(1, ref splitLine))
            {
                return;
            }

            var parameters = new JObject
            {
                ["elements"] = JObject.FromObject(elements)["elements"],
                ["line"] = new JObject
                {
                    ["begin"] = new JObject { ["x"] = splitLine.From.X, ["y"] = splitLine.From.Y },
                    ["end"] = new JObject { ["x"] = splitLine.To.X, ["y"] = splitLine.To.Y }
                }
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var parts = new DataTree<object>();
            var notCut = new List<bool>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "results");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                parts.EnsurePath(path);

                if (item["elements"] is JArray splitParts)
                {
                    foreach (var part in splitParts)
                    {
                        parts.Add(
                            ElementsStructuredGetterComponent.ElementIdOf(part),
                            path);
                    }
                }

                notCut.Add(item.Value<bool?>("notCut") ?? false);
                errors.Add(
                    JsonOutputHelp.IsError(item)
                        ? JsonOutputHelp.ErrorMessage(item)
                        : "");
            }

            da.SetDataTree(
                0,
                parts);
            da.SetDataList(
                1,
                notCut);
            da.SetDataList(
                2,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SplitElements;

        public override Guid ComponentGuid =>
            new Guid("f09244e2-380a-42b6-8736-68afe7f237d4");
    }
}
