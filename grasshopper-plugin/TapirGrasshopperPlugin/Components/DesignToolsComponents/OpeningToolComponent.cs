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
    // The Door and the Window tool: read, write and create doors or windows
    // with the settings of the Door/Window Settings dialog, from
    // ToolSettings/Door.json or Window.json. OwnerWall and CenterOffset place
    // a new opening (the wall it is cut into and the distance of its center
    // from the wall's start point). Create<Type>s and Modify<Type>s share one
    // apply function on the Add-On side (ApplyWindowOrDoorDetails), so they
    // take the same settings.
    public abstract class OpeningToolComponent : ElementToolComponent
    {
        // "door" or "window", for messages.
        protected abstract string ElementNoun { get; }

        protected override bool UseLabels => true;

        // The keys of WindowOrDoorDetailSchemaFields (ExtendedElementCommands.cpp),
        // which the Create and the Modify item schemas of doors and windows take.
        private static readonly string[] DetailKeys =
        {
            "aboveViewLinePen", "aboveViewLineTypeId", "belowViewLinePen", "belowViewLineTypeId",
            "centerOffset", "cutLineTypeId", "displayOption", "height", "lineTypeId", "oSide", "pen",
            "reflected", "refSide", "reveal", "revealDepthOffset", "sectionContourPen",
            "sectionFillBackgroundPen", "sectionFillId", "sectionFillPen", "sillHeight",
            "useObjectLineTypes", "useObjectPens", "useObjectSectionAttributes", "width"
        };

        private static readonly ISet<string> CreateOpeningKeys = new HashSet<string>(
            DetailKeys.Concat(new[] { "favoriteName", "libraryPartName", "ownerWallId" }));

        private static readonly ISet<string> ModifyOpeningKeys = new HashSet<string>(DetailKeys);

        protected override ISet<string> CreateKeys => CreateOpeningKeys;
        protected override ISet<string> ModifyKeys => ModifyOpeningKeys;

        // The Door/Window Settings dialog's panels, in its order; append-only
        // (see ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Preview and Positioning",
            "Floor Plan and Section"
        };

        protected OpeningToolComponent(
            string name,
            string description)
            : base(
                name,
                description)
        {
        }

        protected override int GeometryInputCount => 2;

        protected override void AddGeometryInputs()
        {
            InGenerics(
                "OwnerWall",
                $"The wall each new {ElementNoun} is placed in (Create): one for all {ElementNoun}s or one per " +
                $"{ElementNoun}.");
            InNumbers(
                "CenterOffset",
                $"Distance of each new {ElementNoun}'s center from the start point of its wall (Create): one for " +
                $"all {ElementNoun}s or one per {ElementNoun}.");
        }

        protected override void AddGeometryOutputs()
        {
            OutGenerics(
                "OwnerWall",
                $"The wall each {ElementNoun} is placed in.");
            OutNumbers(
                "CenterOffset",
                $"Distance of each {ElementNoun}'s center from the start point of its wall.");
        }

        // One opening per item of the longer input; the other one may hold a
        // single item, used for every opening (e.g. several in one wall).
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
                    $"per {ElementNoun}, or one of them for all {ElementNoun}s.");
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
    }
}
