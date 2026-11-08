using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Column tool: reads, writes and creates columns with the settings of
    // the Column Settings dialog, from ToolSettings/Column.json. Origin places
    // a new column (or moves one this component created before); its Z is the
    // absolute elevation of the column's base, as in the Origin read by Refresh.
    public class ColumnsComponent : ElementToolComponent
    {
        protected override string ToolName => "Column";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateColumns";
        protected override string CreateArrayKey => "columnsData";
        protected override string ModifyCommand => "ModifyColumns";
        protected override string ModifyArrayKey => "columnsWithDetails";

        // The item properties of the CreateColumns input schema
        // (ElementCreationCommands.cpp); layerIndex and id are written by
        // the LayerIndex and ElementID inputs instead.
        private static readonly ISet<string> CreateColumnsKeys = new HashSet<string>
        {
            "axisRotationAngle", "buildingMaterialId", "circleBased", "coordinates", "coreAnchor", "depth",
            "favoriteName", "floorIndex", "height", "isSlanted", "isWidthAndHeightLinked", "profileId",
            "slantAngle", "slantDirectionAngle", "veneer", "width"
        };

        // The item properties of the ModifyColumns input schema
        // (ExtendedElementCommands.cpp); a setting CreateColumns does not take
        // is written by ModifyColumns right after the column is created.
        private static readonly ISet<string> ModifyColumnsKeys = new HashSet<string>
        {
            "axisRotationAngle", "bottomOffset", "buildingMaterialId", "circleBased", "coreAnchor", "coverFill",
            "cutFillBackgroundPen", "cutFillPen", "depth", "floorIndex", "height", "isFlipped", "isSlanted",
            "isWidthAndHeightLinked", "origin", "profileId", "relativeTopStory", "slantAngle",
            "slantDirectionAngle", "topOffset", "veneer", "width", "wrapping", "zCoordinate"
        };

        // ModifyColumns takes the origin and the base elevation, so Create
        // updates the columns this component made before instead of making
        // new ones (#521).
        protected override bool CanUpdateCreatedElements => true;

        protected override ISet<string> CreateKeys => CreateColumnsKeys;
        protected override ISet<string> ModifyKeys => ModifyColumnsKeys;

        // Origin's Z is the base elevation of a new column, so on Create it
        // wins over the elevation in the JSON.
        protected override IEnumerable<string> GeometrySettingKeys { get; } = new[] { "zCoordinate", "bottomOffset" };

        // The Column Settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Positioning",
            "End Cuts and Reference Axis",
            "Structure",
            "Floor Plan and Section"
        };

        protected override void PrepareSettings(
            JObject settings,
            ISet<string> notes)
        {
            // Bottom Elevation and bottomOffset both set the base: ModifyColumns
            // applies bottomOffset after the elevation, undoing it. A JSON read
            // by Refresh carries both, so the Bottom Elevation is used.
            if (settings["zCoordinate"] != null && settings["bottomOffset"] != null)
            {
                settings.Remove("bottomOffset");
                notes.Add("bottomOffset is not used together with Bottom Elevation; Bottom Elevation sets the base.");
            }

            // As in the Column Settings dialog, a top-linked column's height is
            // not editable: Archicad computes it from the stories, and
            // ModifyColumns applies relativeTopStory after height, undoing it.
            var relativeTopStory = settings["relativeTopStory"];
            if (settings["height"] != null &&
                relativeTopStory != null &&
                relativeTopStory.Type != JTokenType.Null &&
                (double) relativeTopStory != 0)
            {
                settings.Remove("height");
                notes.Add("Column Height is not used for a top-linked column; set Top Link to 0 (Not Linked) to give a fixed height.");
            }
        }

        public ColumnsComponent()
            : base(
                "Columns",
                "Reads (Refresh), writes (Execute) and creates (Create) columns, with the settings of the " +
                "Column Settings dialog as one JSON object per dialog panel. A new column starts from the Column " +
                "tool defaults, or from FavoriteName, and the given settings override them. Origin places a new " +
                "column: its Z is the absolute elevation of the column's base. Create updates the columns this " +
                "component created before (kept in the .gh file) instead of creating new ones; a column deleted " +
                "since then is reported in Errors, not created again. Switch this off or forget the columns in " +
                "the component menu.")
        {
        }

        protected override int GeometryInputCount => 1;

        protected override void AddGeometryInputs()
        {
            InPoints(
                "Origin",
                "Position of each new column, or of a column this component created before (Create); Z is the " +
                "absolute elevation of its base. The home story " +
                "is derived from Z, or is the Home Story of the Positioning JSON when given.");
        }

        protected override void AddGeometryOutputs()
        {
            OutPoints(
                "Origin",
                "Position of each column; Z is the absolute elevation of its base.");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            // In ModifyColumns' names, so Create can move the columns it
            // updates; PrepareCreateItem turns them into CreateColumns'
            // coordinates for a new column.
            var points = new List<Point3d>();
            da.GetDataList(firstInputIndex, points);
            geometry = points
                .Select(
                    p => new JObject
                    {
                        ["origin"] = new JObject { ["x"] = p.X, ["y"] = p.Y },
                        ["zCoordinate"] = p.Z
                    })
                .ToList();
            return true;
        }

        protected override void PrepareCreateItem(
            JObject item)
        {
            if (!(item["origin"] is JObject origin))
            {
                return;
            }
            item["coordinates"] = new JObject
            {
                ["x"] = origin["x"],
                ["y"] = origin["y"],
                ["z"] = item["zCoordinate"] ?? new JValue(0.0)
            };
            item.Remove("origin");
            item.Remove("zCoordinate");
        }

        protected override void SetGeometryOutputs(
            IGH_DataAccess da,
            int firstOutputIndex,
            List<JObject> details)
        {
            da.SetDataList(firstOutputIndex, details.Select(d => OriginOf(d)));
        }

        private static Point3d? OriginOf(
            JObject details)
        {
            if (!(details?["origin"] is JObject origin))
            {
                return null;
            }
            return new Point3d(
                (double) origin["x"],
                (double) origin["y"],
                (double?) details["zCoordinate"] ?? 0.0);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateColumns;

        public override Guid ComponentGuid =>
            new Guid("5b0c3e7a-9d41-4f2e-b8a6-3c71e0d94a52");
    }
}
