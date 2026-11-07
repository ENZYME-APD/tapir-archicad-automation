using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.ToolSettings;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Roof tool: reads, writes and creates roofs with the settings of the
    // Roof Settings dialog, from ToolSettings/Roof.json. Outline (a closed
    // polyline) defines a new roof: its X and Y the polygon, its Z the
    // absolute elevation of the pivot line. With a PivotLine the roof is a
    // single-plane roof, else a multi-plane roof whose pivot polygon is the
    // Outline. Execute writes the settings (ModifyRoofs), not the geometry.
    public class RoofsComponent : ElementToolComponent
    {
        protected override string ToolName => "Roof";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateRoofs";
        protected override string CreateArrayKey => "roofsData";
        protected override string ModifyCommand => "ModifyRoofs";
        protected override string ModifyArrayKey => "roofsWithDetails";

        // The item properties of the CreateRoofs and ModifyRoofs input schemas
        // (ExtendedElementCommands.cpp). CreateRoofs takes no favoriteName.
        private static readonly ISet<string> CreateRoofsKeys = new HashSet<string>
        {
            "angle", "buildingMaterialId", "compositeId", "eavesOverhang", "floorIndex", "holes", "level",
            "levels", "pivotLine", "polygonArcs", "polygonCoordinates", "structureType", "thickness"
        };

        private static readonly ISet<string> ModifyRoofsKeys = new HashSet<string>
        {
            "angle", "buildingMaterialId", "compositeId", "eavesOverhang", "floorIndex", "holes", "level",
            "levels", "pivotEdges", "pivotLine", "polygonArcs", "polygonOutline", "structureType", "thickness"
        };

        protected override ISet<string> CreateKeys => CreateRoofsKeys;
        protected override ISet<string> ModifyKeys => ModifyRoofsKeys;

        // The Roof Settings dialog's panels, in the table's order; append-only
        // (see ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Geometry and Positioning",
            "Multi-plane Geometry",
            "Floor Plan and Section",
            "Model"
        };

        public RoofsComponent()
            : base(
                "Roofs",
                "Reads (Refresh), writes (Execute) and creates (Create) roofs, with the settings of the Roof " +
                "Settings dialog as one JSON object per dialog panel. A new roof starts from the Roof tool " +
                "defaults and the given settings override them; Outline gives its polygon and, by its Z, the " +
                "absolute elevation of its pivot line. With a PivotLine it is a single-plane roof (Roof Pitch), " +
                "without one a multi-plane roof whose pivot polygon is the Outline (Multi-plane Geometry). " +
                "Execute writes the settings, not the outline or the pivot line. Holes and curved edges are " +
                "not handled here (CreateRoofs and ModifyRoofs take them).")
        {
        }

        protected override int GeometryInputCount => 2;

        protected override void AddGeometryInputs()
        {
            inManager.AddCurveParameter(
                "Outline",
                "Outline",
                "The polygon of each new roof, a closed polyline (Create): the roof's outline for a single-plane " +
                "roof, the pivot polygon for a multi-plane roof. X and Y give the polygon, the Z of its first " +
                "vertex the absolute elevation of the pivot line (Absolute Pivot Elevation)."
                    .WithTypeName("curveList"),
                GH_ParamAccess.list);
            inManager.AddCurveParameter(
                "PivotLine",
                "PivotLine",
                "The pivot line of each new single-plane roof, a line (Create; only X and Y are used; the roof " +
                "rises on the left of its direction). Give none for multi-plane roofs, or one per Outline."
                    .WithTypeName("curveList"),
                GH_ParamAccess.list);
        }

        protected override void AddGeometryOutputs()
        {
            OutCurves(
                "Outline",
                "The outline of each single-plane roof, the pivot polygon of each multi-plane roof, at the " +
                "elevation of the pivot line (curved edges come back straight).");
            OutCurves(
                "PivotLine",
                "The pivot line of each single-plane roof, at its elevation (null for a multi-plane roof).");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = new List<JObject>();
            var outlines = new List<Curve>();
            var pivotLines = new List<Curve>();
            da.GetDataList(firstInputIndex, outlines);
            da.GetDataList(firstInputIndex + 1, pivotLines);
            if (pivotLines.Count > 0 && pivotLines.Count != outlines.Count)
            {
                this.AddError(
                    $"PivotLine has {pivotLines.Count} items and Outline {outlines.Count}: give none (multi-plane " +
                    "roofs) or one per Outline (single-plane roofs).");
                return false;
            }

            for (var i = 0; i < outlines.Count; i++)
            {
                if (outlines[i] == null ||
                    !outlines[i].TryGetPolyline(out Polyline polyline) ||
                    !polyline.IsClosed ||
                    polyline.Count < 4)
                {
                    this.AddError($"Outline item {i} is not a closed polyline of at least 3 vertices.");
                    return false;
                }

                // The closing vertex repeats the first one; the Add-On closes
                // the polygon itself.
                var vertices = polyline.Take(polyline.Count - 1);
                var roof = new JObject
                {
                    ["polygonCoordinates"] = new JArray(
                        vertices.Select(p => new JObject { ["x"] = p.X, ["y"] = p.Y })),
                    ["level"] = polyline[0].Z
                };

                if (pivotLines.Count > 0)
                {
                    var pivot = pivotLines[i];
                    if (pivot == null || pivot.PointAtStart.DistanceTo(pivot.PointAtEnd) < 1e-9)
                    {
                        this.AddError($"PivotLine item {i} is not a line of nonzero length.");
                        return false;
                    }
                    roof["pivotLine"] = new JObject
                    {
                        ["begCoordinate"] = new JObject { ["x"] = pivot.PointAtStart.X, ["y"] = pivot.PointAtStart.Y },
                        ["endCoordinate"] = new JObject { ["x"] = pivot.PointAtEnd.X, ["y"] = pivot.PointAtEnd.Y }
                    };
                }

                geometry.Add(roof);
            }

            return true;
        }

        protected override void SetGeometryOutputs(
            IGH_DataAccess da,
            int firstOutputIndex,
            List<JObject> details)
        {
            da.SetDataList(firstOutputIndex, details.Select(d => OutlineOf(d)));
            da.SetDataList(firstOutputIndex + 1, details.Select(d => PivotLineOf(d)));
        }

        private static bool IsMultiPlane(
            JObject details)
        {
            return details?["roofClass"]?.ToString() == "MultiPlane";
        }

        private static double ElevationOf(
            JObject details)
        {
            return (double?) details?["zCoordinate"] ?? 0.0;
        }

        private static Curve OutlineOf(
            JObject details)
        {
            var key = IsMultiPlane(details) ? "pivotPolygonOutline" : "polygonOutline";
            if (!(details?[key] is JArray coordinates) || coordinates.Count < 3)
            {
                return null;
            }

            var z = ElevationOf(details);
            var points = coordinates
                .Select(c => new Point3d((double) c["x"], (double) c["y"], z))
                .ToList();
            if (points[0].DistanceTo(points[points.Count - 1]) > 1e-9)
            {
                points.Add(points[0]);
            }
            return new PolylineCurve(points);
        }

        private static Curve PivotLineOf(
            JObject details)
        {
            if (details == null ||
                IsMultiPlane(details) ||
                !(details["pivotLine"]?["begin"] is JObject begin) ||
                !(details["pivotLine"]?["end"] is JObject end))
            {
                return null;
            }

            var z = ElevationOf(details);
            return new LineCurve(
                new Point3d((double) begin["x"], (double) begin["y"], z),
                new Point3d((double) end["x"], (double) end["y"], z));
        }

        // The commands' "level" is the absolute elevation of the pivot line,
        // which GetDetailsOfElements returns as zCoordinate (Absolute Pivot
        // Elevation); its "level" is the offset from the home story. So the
        // absolute value is written as level, and the offset is read-only.
        protected override string SetKeyOf(
            ToolSettingsField field)
        {
            if (field.Get == "zCoordinate")
            {
                return "level";
            }
            if (field.Get == "level")
            {
                return null;
            }
            return field.Set;
        }

        protected override void PrepareSettings(
            JObject settings,
            ISet<string> notes)
        {
            // GetDetailsOfElements returns the roof levels as height and angle,
            // the commands take levelHeight and levelAngle.
            if (settings["levels"] is JArray levels)
            {
                foreach (var level in levels.OfType<JObject>())
                {
                    RenameKey(level, "height", "levelHeight");
                    RenameKey(level, "angle", "levelAngle");
                }
            }
        }

        private static void RenameKey(
            JObject json,
            string from,
            string to)
        {
            if (json[from] != null && json[to] == null)
            {
                json[to] = json[from];
            }
            json.Remove(from);
        }

        private bool _favoriteIgnored;
        private bool _otherRoofClassIgnored;

        protected override void BeforeSolveInstance()
        {
            base.BeforeSolveInstance();
            _favoriteIgnored = false;
            _otherRoofClassIgnored = false;
        }

        // The settings of the other roof class are left out on Create, so the
        // JSON of a multi-plane roof can be given to a single-plane one and
        // the other way round. CreateRoofs takes no favorite.
        protected override void PrepareCreateItem(
            JObject item)
        {
            if (item.Remove("favoriteName") && !_favoriteIgnored)
            {
                _favoriteIgnored = true;
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Warning,
                    "CreateRoofs takes no favorite: FavoriteName was ignored; new roofs start from the Roof tool defaults.");
            }

            var removed = item["pivotLine"] != null
                ? item.Remove("levels") | item.Remove("eavesOverhang")
                : item.Remove("angle");
            if (removed && !_otherRoofClassIgnored)
            {
                _otherRoofClassIgnored = true;
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Roof Pitch is only used for single-plane roofs (with a PivotLine), Eaves Overhang and Roof Levels " +
                    "only for multi-plane roofs; the others were left out on Create.");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateRoofs;

        public override Guid ComponentGuid =>
            new Guid("4a8e2f61-93c7-4d5b-b0e2-7c1f9a36d584");
    }
}
