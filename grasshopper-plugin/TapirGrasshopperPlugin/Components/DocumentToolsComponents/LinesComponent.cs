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
    // The Line tool: reads and creates lines with the settings of the Line
    // tool's settings dialog, from ToolSettings/Line.json. StartPoint and
    // EndPoint define a new line. There is no ModifyLineElements command, so
    // Execute only writes the Element ID, the layer and the draw order of
    // existing lines.
    public class LinesComponent : ElementToolComponent
    {
        protected override string ToolName => "Line";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateLineElements";
        protected override string CreateArrayKey => "linesData";
        protected override string ModifyCommand => null;
        protected override string ModifyArrayKey => null;

        // The item properties of the CreateLineElements input schema
        // (ElementCreationCommands.cpp, with LineFamilySettingsSchemaProperties).
        private static readonly ISet<string> CreateLineElementsKeys = new HashSet<string>
        {
            "begCoordinate", "endCoordinate", "favoriteName", "floorInd", "id", "layerIndex",
            "linePenIndex", "lineTypeId", "roomSeparator"
        };

        protected override ISet<string> CreateKeys => CreateLineElementsKeys;
        protected override ISet<string> ModifyKeys { get; } = new HashSet<string>();

        // The panels of the Line tool's settings dialog, in its order;
        // append-only (see ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "General Settings"
        };

        public LinesComponent()
            : base(
                "Lines",
                "Reads (Refresh) and creates (Create) lines, with the settings of the Line tool's settings " +
                "dialog as one JSON object per dialog panel. A new line starts from the Line tool defaults, or " +
                "from FavoriteName, and the given settings override them. The story of a new line can be given " +
                "as {\"floorIndex\": ...} in the JSON input. Archicad has no command to modify lines, so Execute " +
                "only writes ElementID, LayerIndex and DrawOrder.",
                GroupNames.DocumentTools)
        {
        }

        protected override int GeometryInputCount => 2;

        protected override void AddGeometryInputs()
        {
            InPoints(
                "StartPoint",
                "Start point of each new line (Create; only X and Y are used).");
            InPoints(
                "EndPoint",
                "End point of each new line (Create; only X and Y are used).");
        }

        protected override void AddGeometryOutputs()
        {
            OutPoints(
                "StartPoint",
                "Start point of each line; Z is the level of its home story.");
            OutPoints(
                "EndPoint",
                "End point of each line; Z is the level of its home story.");
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
                    $"StartPoint has {starts.Count} points and EndPoint {ends.Count}: give one pair per line.");
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

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLineElements;

        public override Guid ComponentGuid =>
            new Guid("3c8e41d2-7f5a-4b19-9d6e-a2b4c57e1f90");
    }
}
