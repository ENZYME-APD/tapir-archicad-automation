using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class RegularizePolygonsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "RegularizePolygons";

        public RegularizePolygonsComponent()
            : base(
                "RegularizePolygons",
                "Regularizes polygons with Archicad's own polygon regularization, before they are used to create " +
                "slabs, zones or fills: removes duplicate points, fixes the orientation, resolves self-intersections " +
                "and touching holes. Each input polygon gives 0, 1 or more valid polygons, optionally with the input " +
                "vertex and edge each result vertex and edge came from. Changes nothing in the project. Line and arc " +
                "segments are kept as they are; other curves are approximated with polylines. Only X and Y are used.",
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddCurveParameter(
                "Outlines",
                "Outlines",
                "The closed outline curve of each polygon.".WithTypeName("curveList"),
                GH_ParamAccess.list);

            inManager.AddCurveParameter(
                "Holes",
                "Holes",
                "The closed curves of the holes of each polygon (one branch per polygon). Optional."
                    .WithTypeName("curveTree"),
                GH_ParamAccess.tree);

            inManager.AddBooleanParameter(
                "WithAncestry",
                "WithAncestry",
                ("Also return for every vertex and edge of the results which input vertex or edge it came from. " +
                "Defaults to false. Optional.")
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutCurveTree(
                "Outlines",
                "The outline of each valid polygon made of each input polygon (one branch per input polygon).");

            OutCurveTree(
                "Holes",
                "The holes of each valid polygon (branch {i;j} for the j-th result of the i-th input polygon).");

            OutTextTree(
                "Ancestry",
                "Only with WithAncestry: for each valid polygon a JSON object of its contours (the outline first, " +
                "then the holes), each with the input vertex and edge (contourIndex, index; -1 when Archicad " +
                "created it) of every vertex and edge (one branch per input polygon).");

            OutErrorMessages(
                "Error message of each input polygon (empty when it was regularized).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<Curve> outlines))
            {
                return;
            }

            da.GetDataTree(1, out GH_Structure<GH_Curve> holeTree);
            var hasHoles = holeTree != null && holeTree.DataCount > 0;
            if (hasHoles &&
                holeTree.Branches.Count != outlines.Count)
            {
                this.AddError("Holes needs one branch per curve of Outlines.");
                return;
            }

            var polygons = new JArray();
            for (var i = 0; i < outlines.Count; i++)
            {
                if (!PolygonHelp.TryToCoordinates(
                        outlines[i],
                        true,
                        out var coordinates,
                        out var arcs,
                        out var error))
                {
                    this.AddError($"Outlines item {i}: {error}.");
                    return;
                }

                var polygon = new JObject { ["polygonCoordinates"] = coordinates };
                if (arcs.Count > 0)
                {
                    polygon["polygonArcs"] = arcs;
                }

                if (hasHoles)
                {
                    var holes = new JArray();
                    foreach (var holeCurve in holeTree.Branches[i])
                    {
                        if (holeCurve?.Value == null)
                        {
                            continue;
                        }

                        if (!PolygonHelp.TryToCoordinates(
                                holeCurve.Value,
                                true,
                                out var holeCoordinates,
                                out var holeArcs,
                                out var holeError))
                        {
                            this.AddError($"Holes of polygon {i}: {holeError}.");
                            return;
                        }

                        var hole = new JObject { ["polygonCoordinates"] = holeCoordinates };
                        if (holeArcs.Count > 0)
                        {
                            hole["polygonArcs"] = holeArcs;
                        }

                        holes.Add(hole);
                    }

                    if (holes.Count > 0)
                    {
                        polygon["holes"] = holes;
                    }
                }

                polygons.Add(polygon);
            }

            var parameters = new JObject { ["polygons"] = polygons };
            var withAncestry = false;
            if (da.GetData(2, ref withAncestry))
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

            var resultOutlines = new DataTree<Curve>();
            var resultHoles = new DataTree<Curve>();
            var ancestry = new DataTree<string>();
            var errors = new List<string>();

            var results = JsonOutputHelp.Items(
                response,
                "results");
            for (var i = 0; i < results.Count; i++)
            {
                var result = results[i];
                var path = new GH_Path(i);
                resultOutlines.EnsurePath(path);
                ancestry.EnsurePath(path);

                if (JsonOutputHelp.IsError(result))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(result));
                    continue;
                }

                errors.Add("");
                if (!(result["polygons"] is JArray resultPolygons))
                {
                    continue;
                }

                for (var j = 0; j < resultPolygons.Count; j++)
                {
                    var resultPolygon = resultPolygons[j];
                    resultOutlines.Add(
                        PolygonHelp.ToCurve(
                            resultPolygon["polygonCoordinates"],
                            resultPolygon["polygonArcs"],
                            true),
                        path);

                    var holePath = new GH_Path(i, j);
                    resultHoles.EnsurePath(holePath);
                    if (resultPolygon["holes"] is JArray resultPolygonHoles)
                    {
                        foreach (var hole in resultPolygonHoles)
                        {
                            resultHoles.Add(
                                PolygonHelp.ToCurve(
                                    hole["polygonCoordinates"],
                                    hole["polygonArcs"],
                                    true),
                                holePath);
                        }
                    }

                    if (resultPolygon["ancestry"] is JObject polygonAncestry)
                    {
                        ancestry.Add(polygonAncestry.ToString(Formatting.None), path);
                    }
                }
            }

            da.SetDataTree(0, resultOutlines);
            da.SetDataTree(1, resultHoles);
            da.SetDataTree(2, ancestry);
            da.SetDataList(3, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RegularizePolygons;

        public override Guid ComponentGuid =>
            new Guid("e439fb3d-8dd0-4501-ab16-12920f22f7e3");
    }
}
