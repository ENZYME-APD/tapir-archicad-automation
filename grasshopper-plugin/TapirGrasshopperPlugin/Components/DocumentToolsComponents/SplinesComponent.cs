using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Components.DesignToolsComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DocumentToolsComponents
{
    // The Spline tool: reads and creates splines with the settings of the
    // Spline Settings dialog, from ToolSettings/Spline.json. Polyline defines a
    // new spline: a polyline whose vertices the spline passes through (only
    // X and Y are used; Archicad smooths the curve itself). There is no
    // ModifySplines command: Execute writes the pen, line type, zone boundary
    // and story of existing splines with SetDetailsOfElements, which cannot
    // change their points or closed state, and the Element ID, the layer and
    // the draw order.
    public class SplinesComponent : ElementToolComponent
    {
        protected override string ToolName => "Spline";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateSplines";
        protected override string CreateArrayKey => "splinesData";
        protected override string ModifyCommand => null;
        protected override string ModifyArrayKey => null;

        // The item properties of the CreateSplines input schema
        // (ElementCreationCommands.cpp).
        private static readonly ISet<string> CreateSplinesKeys = new HashSet<string>
        {
            "closed", "coordinates", "favoriteName", "floorInd", "id", "layerIndex", "linePenIndex",
            "lineTypeId", "roomSeparator"
        };

        protected override ISet<string> CreateKeys => CreateSplinesKeys;
        protected override ISet<string> ModifyKeys { get; } = new HashSet<string>();

        // What SetDetailsOfElements writes on a spline (ElementCommands.cpp,
        // API_SplineID); its points and closed state are read-only there.
        protected override ISet<string> TypeSpecificDetailsKeys { get; } = new HashSet<string>
        {
            "linePenIndex", "lineTypeId", "roomSeparator"
        };

        // The Spline Settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "General Settings"
        };

        public SplinesComponent()
            : base(
                "Splines",
                "Reads (Refresh) and creates (Create) splines, with the settings of the Spline Settings dialog " +
                "as one JSON object per dialog panel. A new spline starts from the Spline tool defaults, or from " +
                "FavoriteName, and the given settings override them. Only auto-smoothed splines are created: " +
                "Archicad calculates the curve through the given points. The story can be given as " +
                "{\"floorIndex\": ...} in any JSON input. Execute writes Pen, Line Type, Zone Boundary, the story, " +
                "ElementID, LayerIndex and DrawOrder of existing splines; Archicad cannot change their points " +
                "or whether they are closed.",
                GroupNames.DocumentTools)
        {
        }

        protected override int GeometryInputCount => 1;

        protected override void AddGeometryInputs()
        {
            inManager.AddCurveParameter(
                "Polyline",
                "Polyline",
                ("A polyline per new spline whose vertices the spline passes through (Create; at least 3 " +
                 "points; only X and Y are used). A closed polyline makes a closed spline; for an open one " +
                 "\"closed\" in GeneralSettings_JSON decides.")
                    .WithTypeName("curveList"),
                GH_ParamAccess.list);
        }

        protected override void AddGeometryOutputs()
        {
            OutCurves(
                "Polyline",
                "A polyline through the points of each spline, closed for a closed spline, at the elevation of " +
                "its story; it can be fed back to Polyline.");
            OutCurves(
                "Curve",
                "An interpolated curve through the points of each spline, at the elevation of its story. It " +
                "approximates the spline: Archicad's own smoothing may differ slightly.");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = new List<JObject>();
            var curves = new List<Curve>();
            da.GetDataList(firstInputIndex, curves);
            for (var i = 0; i < curves.Count; i++)
            {
                if (curves[i] == null ||
                    !curves[i].TryGetPolyline(out Polyline polyline))
                {
                    this.AddError($"Polyline item {i} is not a polyline.");
                    return false;
                }

                // A closed polyline repeats its first point at the end; the
                // spline must not (CreateSplines).
                var points = polyline.ToList();
                var closed = polyline.IsClosed && points.Count > 1;
                if (closed)
                {
                    points.RemoveAt(points.Count - 1);
                }
                if (points.Count < 3)
                {
                    this.AddError($"Polyline item {i} has fewer than 3 points; a spline needs at least 3.");
                    return false;
                }

                var item = new JObject
                {
                    ["coordinates"] = new JArray(
                        points.Select(p => new JObject { ["x"] = p.X, ["y"] = p.Y }))
                };
                if (closed)
                {
                    item["closed"] = true;
                }
                geometry.Add(item);
            }

            return true;
        }

        protected override void SetGeometryOutputs(
            IGH_DataAccess da,
            int firstOutputIndex,
            List<JObject> details)
        {
            da.SetDataList(
                firstOutputIndex,
                details.Select(d => PointsOf(d)));
            da.SetDataList(
                firstOutputIndex + 1,
                details.Select(d => CurveOf(d)));
        }

        private static List<Point3d> SplinePoints(
            JObject details,
            out bool closed)
        {
            closed = (bool?) details?["closed"] ?? false;
            if (!(details?["coordinates"] is JArray coordinates) ||
                coordinates.Count < 2)
            {
                return null;
            }

            var z = (double?) details["zCoordinate"] ?? 0.0;
            return coordinates
                .Select(c => new Point3d((double) c["x"], (double) c["y"], z))
                .ToList();
        }

        private static Curve PointsOf(
            JObject details)
        {
            var points = SplinePoints(details, out var closed);
            if (points == null)
            {
                return null;
            }

            if (closed)
            {
                points.Add(points[0]);
            }
            return new PolylineCurve(points);
        }

        private static Curve CurveOf(
            JObject details)
        {
            var points = SplinePoints(details, out var closed);
            if (points == null)
            {
                return null;
            }

            return Curve.CreateInterpolatedCurve(
                points,
                3,
                closed ? CurveKnotStyle.ChordPeriodic : CurveKnotStyle.Chord);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSplines;

        public override Guid ComponentGuid =>
            new Guid("6b3f2e1a-9c47-4d2b-8e5a-1f0c7d9a4b36");
    }
}
