using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Door tool: reads, writes and creates doors with the settings of the
    // Door Settings dialog, from ToolSettings/Door.json. OwnerWall and
    // CenterOffset place a new door (the wall it is cut into and the distance
    // of its center from the wall's start point). CreateDoors and ModifyDoors
    // share one apply function on the Add-On side (ApplyWindowOrDoorDetails),
    // so they take the same settings.
    public class DoorsComponent : ElementToolComponent
    {
        protected override string ToolName => "Door";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateDoors";
        protected override string CreateArrayKey => "doorsData";
        protected override string ModifyCommand => "ModifyDoors";
        protected override string ModifyArrayKey => "doorsWithDetails";

        // The keys of WindowOrDoorDetailSchemaFields (ExtendedElementCommands.cpp),
        // which both the CreateDoors and the ModifyDoors item schemas take.
        private static readonly string[] DetailKeys =
        {
            "aboveViewLinePen", "aboveViewLineTypeId", "belowViewLinePen", "belowViewLineTypeId",
            "centerOffset", "cutLineTypeId", "displayOption", "height", "lineTypeId", "oSide", "pen",
            "reflected", "refSide", "reveal", "revealDepthOffset", "sectionContourPen",
            "sectionFillBackgroundPen", "sectionFillId", "sectionFillPen", "sillHeight",
            "useObjectLineTypes", "useObjectPens", "useObjectSectionAttributes", "width"
        };

        private static readonly ISet<string> CreateDoorsKeys = new HashSet<string>(
            DetailKeys.Concat(new[] { "favoriteName", "libraryPartName", "ownerWallId" }));

        private static readonly ISet<string> ModifyDoorsKeys = new HashSet<string>(DetailKeys);

        protected override ISet<string> CreateKeys => CreateDoorsKeys;
        protected override ISet<string> ModifyKeys => ModifyDoorsKeys;

        // The Door Settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Preview and Positioning",
            "Floor Plan and Section"
        };

        public DoorsComponent()
            : base(
                "Doors",
                "Reads (Refresh), writes (Execute) and creates (Create) doors, with the settings of the " +
                "Door Settings dialog as one JSON object per dialog panel. A new door is placed in OwnerWall at " +
                "CenterOffset; it starts from the Door tool defaults, or from FavoriteName, and the given " +
                "settings override them.")
        {
        }

        protected override int GeometryInputCount => 2;

        protected override void AddGeometryInputs()
        {
            InGenerics(
                "OwnerWall",
                "The wall each new door is placed in (Create): one for all doors or one per door.");
            InNumbers(
                "CenterOffset",
                "Distance of each new door's center from the start point of its wall (Create): one for all " +
                "doors or one per door.");
        }

        protected override void AddGeometryOutputs()
        {
            OutGenerics(
                "OwnerWall",
                "The wall each door is placed in.");
            OutNumbers(
                "CenterOffset",
                "Distance of each door's center from the start point of its wall.");
        }

        // One door per item of the longer input; the other one may hold a
        // single item, used for every door (e.g. several doors in one wall).
        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = null;
            var wrappers = new List<GH_ObjectWrapper>();
            var offsets = new List<double>();
            da.GetDataList(firstInputIndex, wrappers);
            da.GetDataList(firstInputIndex + 1, offsets);
            var walls = wrappers.Select(w => ElementGuid.CreateFromWrapper(w)?.Guid).ToList();

            if (walls.Count == 0 && offsets.Count == 0)
            {
                geometry = new List<JObject>();
                return true;
            }
            if (walls.Count == 0 || offsets.Count == 0)
            {
                this.AddError("Create needs both OwnerWall and CenterOffset.");
                return false;
            }

            var count = Math.Max(walls.Count, offsets.Count);
            if ((walls.Count != 1 && walls.Count != count) ||
                (offsets.Count != 1 && offsets.Count != count))
            {
                this.AddError(
                    $"OwnerWall has {walls.Count} items and CenterOffset {offsets.Count}: give one of each " +
                    "per door, or one of them for all doors.");
                return false;
            }

            geometry = new List<JObject>();
            for (var i = 0; i < count; i++)
            {
                var wall = walls.Count == 1 ? walls[0] : walls[i];
                if (wall == null)
                {
                    this.AddError($"OwnerWall item {(walls.Count == 1 ? 0 : i)} is not a valid element GUID.");
                    geometry = null;
                    return false;
                }
                geometry.Add(new JObject
                {
                    ["ownerWallId"] = ElementIdObject(wall),
                    ["centerOffset"] = offsets.Count == 1 ? offsets[0] : offsets[i]
                });
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
                details.Select(d => OwnerWallOf(d)));
            da.SetDataList(
                firstOutputIndex + 1,
                details.Select(d => (double?) d?["centerOffset"]));
        }

        private static ElementGuidWrapper OwnerWallOf(
            JObject details)
        {
            var guid = details?["ownerElementId"]?["guid"]?.ToString();
            return guid == null
                ? null
                : new ElementGuidWrapper { ElementId = new ElementGuid { Guid = guid } };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateDoors;

        public override Guid ComponentGuid =>
            new Guid("5d3c6a1e-7f42-4b8e-9c1d-2a6e0f4b8d37");
    }
}
