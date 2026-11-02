using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class RegularizePolylinesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "RegularizePolylines";

        public RegularizePolylinesComponent()
            : base(
                "RegularizePolylines",
                "Regularizes polylines with Archicad's own polyline regularization: removes repeated consecutive " +
                "points. The result may be empty, optionally with the input point and segment each result point " +
                "and segment came from. Changes nothing in the project. Line and arc segments are kept as they " +
                "are; other curves are approximated with polylines. Only X and Y are used.",
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddCurveParameter(
                "Polylines",
                "Polylines",
                "The polyline curves to regularize.".WithTypeName("curveList"),
                GH_ParamAccess.list);

            inManager.AddBooleanParameter(
                "WithAncestry",
                "WithAncestry",
                ("Also return for every point and segment of the results which input point or segment it came " +
                "from. Defaults to false. Optional.")
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutCurves(
                "Polylines",
                "The regularized polyline of each input (null when nothing valid is left of it).");

            OutIntegerTree(
                "VertexAncestry",
                "Only with WithAncestry: for each point of the result the index of the input point it came from, " +
                "-1 when Archicad created it (one branch per polyline).");

            OutIntegerTree(
                "EdgeAncestry",
                "Only with WithAncestry: for each segment of the result the index of the input segment it came " +
                "from, -1 when Archicad created it (one branch per polyline).");

            OutErrorMessages(
                "Error message of each input polyline (empty when it was regularized).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<Curve> curves))
            {
                return;
            }

            var polylines = new JArray();
            for (var i = 0; i < curves.Count; i++)
            {
                if (!PolygonHelp.TryToCoordinates(
                        curves[i],
                        false,
                        out var coordinates,
                        out var arcs,
                        out var error))
                {
                    this.AddError($"Polylines item {i}: {error}.");
                    return;
                }

                var polyline = new JObject { ["coordinates"] = coordinates };
                if (arcs.Count > 0)
                {
                    polyline["arcs"] = arcs;
                }

                polylines.Add(polyline);
            }

            var parameters = new JObject { ["polylines"] = polylines };
            var withAncestry = false;
            if (da.GetData(1, ref withAncestry))
            {
                parameters["withAncestry"] = withAncestry;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var resultCurves = new List<Curve>();
            var vertexAncestry = new DataTree<int>();
            var edgeAncestry = new DataTree<int>();
            var errors = new List<string>();

            var results = JsonOutputHelp.Items(
                response,
                "results");
            for (var i = 0; i < results.Count; i++)
            {
                var result = results[i];
                var path = new GH_Path(i);
                vertexAncestry.EnsurePath(path);
                edgeAncestry.EnsurePath(path);

                if (JsonOutputHelp.IsError(result))
                {
                    resultCurves.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(result));
                    continue;
                }

                resultCurves.Add(
                    PolygonHelp.ToCurve(
                        result["coordinates"],
                        result["arcs"],
                        false));
                AddIndices(vertexAncestry, result["ancestry"]?["vertices"], path);
                AddIndices(edgeAncestry, result["ancestry"]?["edges"], path);
                errors.Add("");
            }

            da.SetDataList(0, resultCurves);
            da.SetDataTree(1, vertexAncestry);
            da.SetDataTree(2, edgeAncestry);
            da.SetDataList(3, errors);
        }

        private static void AddIndices(
            DataTree<int> tree,
            JToken items,
            GH_Path path)
        {
            if (!(items is JArray array))
            {
                return;
            }

            foreach (var item in array)
            {
                tree.Add(item.Value<int?>("index") ?? -1, path);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RegularizePolylines;

        public override Guid ComponentGuid =>
            new Guid("c426c3a7-ba78-4a06-9aa9-5a90f618b880");
    }
}
