using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetSplineMaxDistancesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetSplineMaxDistances";

        public GetSplineMaxDistancesComponent()
            : base(
                "GetSplineMaxDistances",
                "Measures how far the given points are from a segment of a Spline element, between two of its " +
                "fit points, with its own Bezier handles: returns the largest distance, for example to check how " +
                "well a polyline follows the spline. Changes nothing in the project.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of Spline elements (repeat one to measure several of its segments).");

            InIntegers(
                "SegmentIndices",
                "The segment of each spline between its fit points SegmentIndex and SegmentIndex + 1, from 0 to " +
                "the number of fit points - 2 (input only 1 to use the same segment for all).");

            inManager.AddPointParameter(
                "Points",
                "Points",
                "The points to measure from each segment (one branch per spline, only X and Y are used; input " +
                "only 1 branch to use the same points for all).",
                GH_ParamAccess.tree);
        }

        protected override void AddOutputs()
        {
            OutNumbers(
                "MaxDistances",
                "The largest distance of the points from each spline segment, aligned with ElementGuids (null " +
                "where it failed).");

            OutErrorMessages(
                "Error message of each spline (empty when it was measured).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out var elementIds))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<int> segmentIndices) ||
                !this.CheckSizes(
                    elementIds.Count,
                    "ElementGuids",
                    ("SegmentIndices", segmentIndices.Count)))
            {
                return;
            }

            da.TryGetTree(
                2,
                out GH_Structure<GH_Point> points);
            var branchCount = points?.Branches.Count ?? 0;
            if (branchCount != 1 && branchCount != elementIds.Count)
            {
                this.AddError(
                    "The number of branches in the Points input must be 1 or equal to the size of the input ElementGuids.");
                return;
            }

            var splines = new JArray();
            for (var i = 0; i < elementIds.Count; i++)
            {
                var coordinates = new JArray();
                foreach (var ghPoint in points.Branches[branchCount == 1 ? 0 : i])
                {
                    coordinates.Add(
                        new JObject
                        {
                            ["x"] = ghPoint.Value.X,
                            ["y"] = ghPoint.Value.Y
                        });
                }

                splines.Add(
                    new JObject
                    {
                        ["elementId"] = elementIds[i],
                        ["segmentIndex"] = segmentIndices.At(i),
                        ["coordinates"] = coordinates
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["splines"] = splines },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var maxDistances = new List<double?>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "results"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    maxDistances.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                maxDistances.Add(item["maxDistance"]?.Value<double>());
                errors.Add("");
            }

            da.SetDataList(0, maxDistances);
            da.SetDataList(1, errors);
        }

        // Shares the icon of RegularizePolygons, the other geometry helper.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RegularizePolygons;

        public override Guid ComponentGuid =>
            new Guid("c03e7ff9-8e04-43bc-ae76-d9b07a85775d");
    }
}
