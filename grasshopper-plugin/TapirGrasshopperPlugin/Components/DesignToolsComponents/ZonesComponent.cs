using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Zone tool: reads, writes and creates zones with the settings of the
    // Zone Settings dialog, from ToolSettings/Zone.json. A new zone is either
    // manual, from its Boundary (a polyline; only X and Y are used), or
    // automatic, found around its ReferencePoint by Archicad. ModifyZones
    // keeps the polygon, so Create always makes new zones; the geometry of an
    // existing zone is not changed by Execute.
    public class ZonesComponent : ElementToolComponent
    {
        protected override string ToolName => "Zone";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateZones";
        protected override string CreateArrayKey => "zonesData";
        protected override string ModifyCommand => "ModifyZones";
        protected override string ModifyArrayKey => "zonesWithDetails";

        // The item properties of the CreateZones and ModifyZones input schemas
        // (ElementCreationCommands.cpp).
        private static readonly ISet<string> CreateZonesKeys = new HashSet<string>
        {
            "allowOverlap", "bottomOffset", "categoryAttributeId", "contourLine", "coverFill", "favoriteName",
            "fixedStampAngle", "floorIndex", "floorThickness", "geometry", "height", "id", "name", "numberStr",
            "relativeTopStory", "stampAngle", "stampPosition", "topOffset"
        };

        private static readonly ISet<string> ModifyZonesKeys = new HashSet<string>
        {
            "bottomOffset", "categoryAttributeId", "fixedStampAngle", "floorThickness", "height", "name",
            "numberStr", "relativeTopStory", "stampAngle", "stampPosition", "topOffset"
        };

        // ModifyZones does not take the contour line and the cover fill;
        // SetDetailsOfElements' ZoneSettings does.
        private static readonly ISet<string> ZoneTypeSpecificDetailsKeys = new HashSet<string>
        {
            "contourLine", "coverFill"
        };

        protected override ISet<string> CreateKeys => CreateZonesKeys;
        protected override ISet<string> ModifyKeys => ModifyZonesKeys;
        protected override ISet<string> TypeSpecificDetailsKeys => ZoneTypeSpecificDetailsKeys;

        // The Zone Settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Name and Positioning",
            "Floor Plan",
            "Zone Stamp"
        };

        public ZonesComponent()
            : base(
                "Zones",
                "Reads (Refresh), writes (Execute) and creates (Create) zones, with the settings of the Zone " +
                "Settings dialog as one JSON object per dialog panel. A new zone starts from the Zone tool " +
                "defaults, or from FavoriteName, and the given settings override them; it needs a Name and a No " +
                "(NameAndPositioning_JSON). Give either Boundary (manual zones) or ReferencePoint (automatic " +
                "zones, found around the point on their Home Story). A new zone overlapping an existing one on " +
                "its story is refused. Execute does not change the polygon of existing zones.")
        {
        }

        protected override int GeometryInputCount => 3;

        protected override void AddGeometryInputs()
        {
            inManager.AddCurveParameter(
                "Boundary",
                "Boundary",
                "The outline of each new manual zone, a polyline of at least 3 vertices (Create; only X and Y " +
                "are used). Give either this or ReferencePoint."
                    .WithTypeName("curveList"),
                GH_ParamAccess.list);
            InPoints(
                "ReferencePoint",
                "The reference point of each new automatic zone, which Archicad finds the zone around (Create; " +
                "only X and Y are used). Give either this or Boundary.");
            InPoints(
                "StampPosition",
                "The position of the zone stamp of each new zone (Create; only X and Y are used): none (the " +
                "first vertex of the boundary or the reference point) or one per zone.");
        }

        protected override void AddGeometryOutputs()
        {
            OutCurves(
                "Boundary",
                "The outline of each zone, at its bottom elevation (arcs are drawn as straight segments, holes " +
                "are left out).");
            OutPoints(
                "ReferencePoint",
                "The reference point of each automatic zone (null for a manual zone), at its bottom elevation.");
            OutPoints(
                "StampPosition",
                "The position of the zone stamp of each zone, at its bottom elevation.");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = new List<JObject>();
            var curves = new List<Curve>();
            var referencePoints = new List<Point3d>();
            var stampPositions = new List<Point3d>();
            da.GetDataList(firstInputIndex, curves);
            da.GetDataList(firstInputIndex + 1, referencePoints);
            da.GetDataList(firstInputIndex + 2, stampPositions);

            if (curves.Count > 0 && referencePoints.Count > 0)
            {
                this.AddError("Give either Boundary or ReferencePoint, not both.");
                return false;
            }

            for (var i = 0; i < curves.Count; i++)
            {
                if (curves[i] == null ||
                    !curves[i].TryGetPolyline(out Polyline polyline))
                {
                    this.AddError($"Boundary item {i} is not a polyline.");
                    return false;
                }

                var points = polyline.ToList();
                if (points.Count > 1 && points[0].DistanceTo(points[points.Count - 1]) < 1e-9)
                {
                    points.RemoveAt(points.Count - 1);
                }
                if (points.Count < 3)
                {
                    this.AddError($"Boundary item {i} has fewer than 3 vertices.");
                    return false;
                }

                geometry.Add(new JObject
                {
                    ["geometry"] = new JObject
                    {
                        ["polygonCoordinates"] = new JArray(points.Select(p => Coordinate(p)))
                    }
                });
            }

            foreach (var point in referencePoints)
            {
                geometry.Add(new JObject
                {
                    ["geometry"] = new JObject { ["referencePosition"] = Coordinate(point) }
                });
            }

            if (stampPositions.Count > 0)
            {
                if (stampPositions.Count != geometry.Count)
                {
                    this.AddError(
                        $"StampPosition has {stampPositions.Count} points for {geometry.Count} zones: give none or one per zone.");
                    return false;
                }
                for (var i = 0; i < geometry.Count; i++)
                {
                    geometry[i]["stampPosition"] = Coordinate(stampPositions[i]);
                }
            }

            return true;
        }

        private static JObject Coordinate(
            Point3d point)
        {
            return new JObject { ["x"] = point.X, ["y"] = point.Y };
        }

        protected override void SetGeometryOutputs(
            IGH_DataAccess da,
            int firstOutputIndex,
            List<JObject> details)
        {
            da.SetDataList(firstOutputIndex, details.Select(d => BoundaryOf(d)));
            da.SetDataList(firstOutputIndex + 1, details.Select(d => PointOf(d, "referencePosition")));
            da.SetDataList(firstOutputIndex + 2, details.Select(d => PointOf(d, "stampPosition")));
        }

        private static Curve BoundaryOf(
            JObject details)
        {
            if (!(details?["polygonOutline"] is JArray coordinates) ||
                coordinates.Count < 3)
            {
                return null;
            }

            var z = (double?) details["zCoordinate"] ?? 0.0;
            var points = coordinates
                .Select(c => new Point3d((double) c["x"], (double) c["y"], z))
                .ToList();
            if (points[0].DistanceTo(points[points.Count - 1]) > 1e-9)
            {
                points.Add(points[0]);
            }
            return new PolylineCurve(points);
        }

        private static Point3d? PointOf(
            JObject details,
            string key)
        {
            if (!(details?[key] is JObject coordinate))
            {
                return null;
            }
            return new Point3d(
                (double) coordinate["x"],
                (double) coordinate["y"],
                (double?) details["zCoordinate"] ?? 0.0);
        }

        // As in the Zone Settings dialog, a top-linked zone's height is not
        // set: Archicad computes it from the stories, and the zone commands
        // apply relativeTopStory after height, so a JSON read by Refresh,
        // which carries both, would otherwise be ambiguous.
        protected override void PrepareSettings(
            JObject settings,
            ISet<string> notes)
        {
            var relativeTopStory = settings["relativeTopStory"];
            if (settings["height"] != null &&
                relativeTopStory != null &&
                relativeTopStory.Type != JTokenType.Null &&
                (double) relativeTopStory != 0)
            {
                settings.Remove("height");
                notes.Add("height is not used for a top-linked zone; set relativeTopStory to 0 to give a fixed height.");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateZones;

        public override Guid ComponentGuid =>
            new Guid("6a1d4c07-3e9b-4f52-9d8e-2b7f0c5e91a4");
    }
}
