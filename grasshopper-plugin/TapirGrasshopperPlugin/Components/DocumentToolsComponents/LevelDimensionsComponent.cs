using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Components.DesignToolsComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DocumentToolsComponents
{
    // The Level Dimension tool: reads and creates level dimensions with the
    // settings of the Level Dimension Settings dialog, from
    // ToolSettings/LevelDimension.json. Position, ParentElementGuid and Level
    // define a new level dimension. There is no ModifyLevelDimensions command,
    // so Execute only writes the Element ID, the layer and the draw order of
    // existing level dimensions.
    public class LevelDimensionsComponent : ElementToolComponent
    {
        protected override string ToolName => "LevelDimension";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateLevelDimensions";
        protected override string CreateArrayKey => "levelDimensionsData";
        protected override string ModifyCommand => null;
        protected override string ModifyArrayKey => null;

        // The item properties of the CreateLevelDimensions input schema
        // (ElementCreationCommands.cpp).
        private static readonly ISet<string> CreateLevelDimensionsKeys = new HashSet<string>
        {
            "angle", "dimForm", "favoriteName", "floorIndex", "id", "layerIndex", "level", "markerSize",
            "needPlus", "origin", "parentElementId", "pen", "position", "textHeight", "textPen"
        };

        protected override ISet<string> CreateKeys => CreateLevelDimensionsKeys;
        protected override ISet<string> ModifyKeys { get; } = new HashSet<string>();

        // The Level Dimension Settings dialog's panels, in its order;
        // append-only (see ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Dimension Type",
            "Text Content",
            "Text Style"
        };

        public LevelDimensionsComponent()
            : base(
                "LevelDimensions",
                "Reads (Refresh) and creates (Create) level dimensions, with the settings of the Level Dimension " +
                "Settings dialog as one JSON object per dialog panel. A new level dimension starts from the Level " +
                "Dimension tool defaults, or from FavoriteName, and the given settings override them. With a " +
                "ParentElementGuid the dimension is associative: it shows the height of the parent element (Mesh, " +
                "Slab, Roof ...) at its position and follows the element when it changes. The story can be given " +
                "as {\"floorIndex\": ...} in any JSON input. Archicad has no command to modify level dimensions, " +
                "so Execute only writes ElementID, LayerIndex and DrawOrder.",
                GroupNames.DocumentTools)
        {
        }

        protected override int GeometryInputCount => 3;

        protected override void AddGeometryInputs()
        {
            InPoints(
                "Position",
                "The location of the marker of each new level dimension (Create; only X and Y are used). With a " +
                "ParentElementGuid it must lie on that element.");
            InGenerics(
                "ParentElementGuid",
                "The element each new level dimension is associated with (Create): none, one for all or one per " +
                "position.");
            InNumbers(
                "Level",
                "A fixed level value of each new level dimension, only used without ParentElementGuid (Create): " +
                "none, one for all or one per position.");
        }

        protected override void AddGeometryOutputs()
        {
            OutPoints(
                "Position",
                "The location of the marker of each level dimension (Z is 0).");
            OutGenerics(
                "ParentElementGuid",
                "The element each level dimension is associated with (null when it is not associative).");
            OutNumberList(
                "Level",
                "The level each level dimension shows: of the parent element, of the floor, or the static value.");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = null;
            var positions = new List<Point3d>();
            da.GetDataList(firstInputIndex, positions);
            var parentWrappers = da.OptionalList<GH_ObjectWrapper>(firstInputIndex + 1);
            var levels = da.OptionalList<double>(firstInputIndex + 2);

            if (!this.TryGetElementIds(
                    parentWrappers,
                    "ParentElementGuid",
                    out var parentIds) ||
                !this.CheckSizes(
                    positions.Count,
                    "Position",
                    ("ParentElementGuid", parentIds.Count),
                    ("Level", levels.Count)))
            {
                return false;
            }

            geometry = new List<JObject>();
            for (var i = 0; i < positions.Count; i++)
            {
                var item = new JObject
                {
                    ["position"] = new JObject { ["x"] = positions[i].X, ["y"] = positions[i].Y }
                };
                if (parentIds.Count > 0)
                {
                    item["parentElementId"] = parentIds.At(i);
                }
                if (levels.Count > 0)
                {
                    item["level"] = levels.At(i);
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
                details.Select(d => JsonOutputHelp.Point(d?["position"])));
            da.SetDataList(
                firstOutputIndex + 1,
                details.Select(d => ItemListHelp.ElementGuidOf(d?["ownerElementId"])));
            da.SetDataList(
                firstOutputIndex + 2,
                details.Select(d => (double?) d?["level"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLevelDimensions;

        public override Guid ComponentGuid =>
            new Guid("07351a11-07c8-4084-a501-9d52b1b2363a");
    }
}
