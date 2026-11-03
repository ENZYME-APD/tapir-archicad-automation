using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Wall tool: reads, writes and creates walls with the settings of
    // the Wall Settings dialog, from ToolSettings/Wall.json. StartPoint and
    // EndPoint define a new wall (only X and Y are used); the Z of the read
    // points is the wall's bottom elevation.
    public abstract class WallsComponentBase : ElementToolComponent
    {
        protected override string ToolName => "Wall";

        protected override string CreateCommand => "CreateWalls";
        protected override string CreateArrayKey => "wallsData";
        protected override string ModifyCommand => "ModifyWalls";
        protected override string ModifyArrayKey => "wallsWithDetails";

        // The item properties of the CreateWalls and ModifyWalls input schemas
        // (ExtendedElementCommands.cpp); a setting the Modify command does not
        // take is written by Create, and the other way round.
        private static readonly ISet<string> CreateWallsKeys = new HashSet<string>
        {
            "aboveViewLinePen", "aboveViewLineTypeId", "alignTexture", "arcAngle", "begCoordinate",
            "belowViewLineTypeId", "buildingMaterialId", "compositeId", "contourLineTypeId", "contourPen",
            "contourPen3D", "displayOption", "endCoordinate", "favoriteName", "flipped", "floorIndex", "height",
            "inheritEndSurface", "linkToSettings", "logHeight", "materialsChained", "offset", "polyCanChange",
            "profileId", "referenceLineLocation", "structureType", "thickness", "useCompositePriority",
            "viewDepthLimitation", "zCoordinate"
        };

        private static readonly ISet<string> ModifyWallsKeys = new HashSet<string>
        {
            "aboveViewLinePen", "aboveViewLineTypeId", "alignTexture", "arcAngle", "begCoordinate",
            "belowViewLineTypeId", "bottomOffset", "buildingMaterialId", "compositeId", "contourLineTypeId",
            "contourPen", "contourPen3D", "cutFillBackgroundPen", "cutFillPen", "displayOption", "endCoordinate",
            "flipped", "geometryType", "height", "inheritEndSurface", "isAutoOnStoryVisibility", "linkToSettings",
            "logHeight", "materialsChained", "offset", "oppositeMaterial", "polyCanChange", "profileId",
            "profileType", "referenceLineLocation", "referenceMaterial", "relativeTopStory", "sideMaterial",
            "slantAlpha", "slantBeta", "structureType", "thickness", "topOffset", "useCompositePriority",
            "viewDepthLimitation", "visibility", "zoneRel"
        };

        protected override ISet<string> CreateKeys => CreateWallsKeys;
        protected override ISet<string> ModifyKeys => ModifyWallsKeys;

        // The Wall Settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Geometry and Positioning",
            "Floor Plan and Section",
            "Model"
        };

        // The values ModifyWalls takes for these keys (its WallModification*
        // enums); GetDetailsOfElements also returns Polygonal and Poly, which
        // cannot be set.
        private static readonly ISet<string> WritableGeometryTypes = new HashSet<string> { "Straight", "Trapezoid" };
        private static readonly ISet<string> WritableProfileTypes = new HashSet<string> { "Normal", "Slanted", "Trapez" };

        protected override void PrepareSettings(
            JObject settings,
            ISet<string> notes)
        {
            var geometryType = settings["geometryType"]?.ToString();
            if (geometryType != null && !WritableGeometryTypes.Contains(geometryType))
            {
                settings.Remove("geometryType");
                notes.Add($"Geometry Method \"{geometryType}\" cannot be set; it was left unchanged.");
            }

            var profileType = settings["profileType"]?.ToString();
            if (profileType != null && !WritableProfileTypes.Contains(profileType))
            {
                settings.Remove("profileType");
                notes.Add($"Wall Complexity \"{profileType}\" cannot be set; it was left unchanged.");
            }

            // As in the Wall Settings dialog, a top-linked wall's height is not
            // editable: Archicad computes it from the stories, and ModifyWalls
            // applies relativeTopStory after height, undoing it.
            var relativeTopStory = settings["relativeTopStory"];
            if (settings["height"] != null &&
                relativeTopStory != null &&
                relativeTopStory.Type != JTokenType.Null &&
                (double) relativeTopStory != 0)
            {
                settings.Remove("height");
                notes.Add("Wall Height is not used for a top-linked wall; set Wall Top to 0 (Not Linked) to give a fixed height.");
            }
        }

        protected WallsComponentBase(
            string name)
            : base(
                name,
                "Reads (Refresh), writes (Execute) and creates (Create) walls, with the settings of the " +
                "Wall Settings dialog as one JSON object per dialog panel. A new wall starts from the Wall " +
                "tool defaults, or from FavoriteName, and the given settings override them.")
        {
        }

        protected override int GeometryInputCount => 2;

        protected override void AddGeometryInputs()
        {
            InPoints(
                "StartPoint",
                "Start point of each new wall (Create; only X and Y are used).");
            InPoints(
                "EndPoint",
                "End point of each new wall (Create; only X and Y are used).");
        }

        protected override void AddGeometryOutputs()
        {
            OutPoints(
                "StartPoint",
                "Start point of each wall; Z is its bottom elevation.");
            OutPoints(
                "EndPoint",
                "End point of each wall; Z is its bottom elevation.");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = null;
            var starts = new List<Point3d>();
            var ends = new List<Point3d>();
            da.GetDataList(firstInputIndex, starts);
            da.GetDataList(firstInputIndex + 1, ends);
            if (starts.Count != ends.Count)
            {
                this.AddError(
                    $"StartPoint has {starts.Count} points and EndPoint {ends.Count}: give one pair per wall.");
                return false;
            }

            geometry = starts.Zip(
                    ends,
                    (start, end) => new JObject
                    {
                        ["begCoordinate"] = new JObject { ["x"] = start.X, ["y"] = start.Y },
                        ["endCoordinate"] = new JObject { ["x"] = end.X, ["y"] = end.Y }
                    })
                .ToList();
            return true;
        }

        protected override void SetGeometryOutputs(
            IGH_DataAccess da,
            int firstOutputIndex,
            List<JObject> details)
        {
            da.SetDataList(firstOutputIndex, details.Select(d => PointOf(d, "begCoordinate")));
            da.SetDataList(firstOutputIndex + 1, details.Select(d => PointOf(d, "endCoordinate")));
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

        // GetDetailsOfElements reports zCoordinate as the absolute bottom
        // elevation, while CreateWalls reads it as the offset from the home
        // story when floorIndex is given too. A wall read by Refresh carries
        // both, so the elevation is left out then; its offset from the home
        // story (bottomOffset) is written right after the wall is created.
        protected override void PrepareCreateItem(
            JObject item)
        {
            if (item["floorIndex"] != null)
            {
                item.Remove("zCoordinate");
            }
        }
    }

    public class WallsComponent : WallsComponentBase
    {
        protected override bool UseLabels => true;

        public WallsComponent()
            : base("Walls")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.WallDetails;

        public override Guid ComponentGuid =>
            new Guid("1ef2911d-e07e-4907-ac68-a16c0b43db6a");
    }

    // The same component with the Add-On's own JSON keys instead of the
    // dialog labels, for testing against the commands.
    public class WallsApiComponent : WallsComponentBase
    {
        protected override bool UseLabels => false;

        public WallsApiComponent()
            : base("Walls_API")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.WallDetails;

        public override Guid ComponentGuid =>
            new Guid("263a50dd-8d3b-431f-b317-7b069b84424a");
    }
}
