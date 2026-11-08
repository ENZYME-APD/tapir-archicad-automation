using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetDrawingNumberingGridLinesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDrawingNumberingGridLines";

        public GetDrawingNumberingGridLinesComponent()
            : base(
                "GetDrawingNumberingGridLines",
                "Gets the lines of the grid of the grid-based drawing numbering of the current layout (start and " +
                "end point, pen index, line type index), for example to find which grid cell a drawing on the " +
                "layout sits in. The current window must be a layout (see ChangeWindow), and only a layout that " +
                "uses grid-based drawing numbering has such a grid.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutCurves(
                "Lines",
                "The lines of the grid, in layout coordinates.");

            OutIntegers(
                "PenIndices",
                "The index of the pen of each line.");

            OutIntegers(
                "LineTypeIndices",
                "The index of the line type of each line.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var lines = new List<Curve>();
            var penIndices = new List<int>();
            var lineTypeIndices = new List<int>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "lines"))
            {
                var begC = JsonOutputHelp.Point(item["begC"]) is Point3d beg
                    ? beg
                    : Point3d.Origin;
                var endC = JsonOutputHelp.Point(item["endC"]) is Point3d end
                    ? end
                    : Point3d.Origin;

                lines.Add(
                    new LineCurve(
                        begC,
                        endC));
                penIndices.Add(item.Value<int?>("penIndex") ?? 0);
                lineTypeIndices.Add(item.Value<int?>("lineTypeIndex") ?? 0);
            }

            da.SetDataList(
                0,
                lines);

            da.SetDataList(
                1,
                penIndices);

            da.SetDataList(
                2,
                lineTypeIndices);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDrawingNumberingGridLines;

        public override Guid ComponentGuid =>
            new Guid("c2af0428-dafe-4891-8fef-7c03769c90ee");
    }
}
