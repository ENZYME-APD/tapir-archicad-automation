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
    // The Label tool: reads, modifies and creates labels with the settings of
    // the Label Settings dialog, from ToolSettings/Label.json. The parent
    // element and the points of the pointer (leader) line define a new label;
    // ModifyLabels cannot move a label or change its parent, so these are only
    // used on Create.
    public class LabelsComponent : ElementToolComponent
    {
        protected override string ToolName => "Label";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateLabels";
        protected override string CreateArrayKey => "labelsData";
        protected override string ModifyCommand => "ModifyLabels";
        protected override string ModifyArrayKey => "labelsWithDetails";

        // The item properties of the CreateLabels input schema
        // (ElementCreationCommands.cpp).
        private static readonly ISet<string> CreateLabelsKeys = new HashSet<string>
        {
            "begCoordinate", "endCoordinate", "favoriteName", "floorInd", "id", "labelClass", "leaderLine",
            "midCoordinate", "parentElementId", "runs", "style", "symbolStyle", "text"
        };

        // The item properties of the ModifyLabels input schema, without
        // elementId (ElementCreationCommands.cpp).
        private static readonly ISet<string> ModifyLabelsKeys = new HashSet<string>
        {
            "leaderLine", "runs", "style", "symbolStyle", "text"
        };

        protected override ISet<string> CreateKeys => CreateLabelsKeys;
        protected override ISet<string> ModifyKeys => ModifyLabelsKeys;

        // The panels of the Label table, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Type and Preview",
            "Text Style",
            "Pointer",
            "Text Label"
        };

        // The read-only members GetDetailsOfElements returns inside style
        // (TextStyleDetails) and leaderLine (LabelLeaderLineDetails), which
        // the Create and Modify commands refuse (additionalProperties false).
        private static readonly string[] ReadOnlyStyleKeys = { "lineCount", "boxWidth", "boxHeight" };
        private static readonly string[] ReadOnlyLeaderLineKeys = { "begCoordinate", "midCoordinate", "endCoordinate" };

        public LabelsComponent()
            : base(
                "Labels",
                "Reads (Refresh), modifies (Execute) and creates (Create) labels, with the settings of the Label " +
                "Settings dialog as one JSON object per dialog panel. A new label starts from the Label tool " +
                "defaults, or from FavoriteName, and the given settings override them. With a ParentElementGuid " +
                "the label is associative; without it BegPoint is needed. Give MidPoint and EndPoint too to " +
                "place the label exactly, otherwise Archicad puts it at its default position. Archicad cannot " +
                "move a label, change its parent or its class (labelClass) through ModifyLabels, so these are " +
                "only used on Create. text, runs and style apply to text labels, symbolStyle to symbol labels. " +
                "The text read by Refresh shows autotexts by their values: written back, they become plain text.",
                GroupNames.DocumentTools)
        {
        }

        protected override int GeometryInputCount => 4;

        protected override void AddGeometryInputs()
        {
            InGenerics(
                "ParentElementGuid",
                "The element each new label is associated with (Create): none (independent labels, BegPoint " +
                "needed), one for all or one per label.");
            InPoints(
                "BegPoint",
                "The beginning point of the pointer line of each new label (Create; only X and Y are used): " +
                "none, one for all or one per label.");
            InPoints(
                "MidPoint",
                "The middle point of the pointer line of each new label (Create; only X and Y are used; " +
                "give it with EndPoint): none, one for all or one per label.");
            InPoints(
                "EndPoint",
                "The end point of the pointer line of each new label (Create; only X and Y are used; give it " +
                "with MidPoint): none, one for all or one per label.");
        }

        protected override void AddGeometryOutputs()
        {
            OutGenerics(
                "ParentElementGuid",
                "The element each label is associated with (null when it is independent).");
            OutPoints(
                "BegPoint",
                "The beginning point of the pointer line of each label (Z is 0).");
            OutPoints(
                "MidPoint",
                "The middle point of the pointer line of each label (Z is 0).");
            OutPoints(
                "EndPoint",
                "The end point of the pointer line of each label (Z is 0).");
        }

        // One label per ParentElementGuid or per BegPoint, whichever has more
        // items; every geometry input holds none, one for all or one per label.
        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = null;
            var parentWrappers = da.OptionalList<GH_ObjectWrapper>(firstInputIndex);
            var begPoints = da.OptionalList<Point3d>(firstInputIndex + 1);
            var midPoints = da.OptionalList<Point3d>(firstInputIndex + 2);
            var endPoints = da.OptionalList<Point3d>(firstInputIndex + 3);

            if (!this.TryGetElementIds(
                    parentWrappers,
                    "ParentElementGuid",
                    out var parentIds))
            {
                return false;
            }

            var byParents = parentIds.Count >= begPoints.Count;
            var count = byParents ? parentIds.Count : begPoints.Count;
            if (!this.CheckSizes(
                    count,
                    byParents ? "ParentElementGuid" : "BegPoint",
                    ("ParentElementGuid", parentIds.Count),
                    ("BegPoint", begPoints.Count),
                    ("MidPoint", midPoints.Count),
                    ("EndPoint", endPoints.Count)))
            {
                return false;
            }

            geometry = new List<JObject>();
            for (var i = 0; i < count; i++)
            {
                var item = new JObject();
                if (parentIds.Count > 0)
                {
                    item["parentElementId"] = parentIds.At(i);
                }
                if (begPoints.Count > 0)
                {
                    item["begCoordinate"] = Coordinate(begPoints.At(i));
                }
                if (midPoints.Count > 0)
                {
                    item["midCoordinate"] = Coordinate(midPoints.At(i));
                }
                if (endPoints.Count > 0)
                {
                    item["endCoordinate"] = Coordinate(endPoints.At(i));
                }
                geometry.Add(item);
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
            da.SetDataList(
                firstOutputIndex,
                details.Select(d => ItemListHelp.ElementGuidOf(d?["ownerElementId"])));
            da.SetDataList(
                firstOutputIndex + 1,
                details.Select(d => JsonOutputHelp.Point(d?["begCoordinate"])));
            da.SetDataList(
                firstOutputIndex + 2,
                details.Select(d => JsonOutputHelp.Point(d?["midCoordinate"])));
            da.SetDataList(
                firstOutputIndex + 3,
                details.Select(d => JsonOutputHelp.Point(d?["endCoordinate"])));
        }

        // A JSON read by Refresh carries read-only members in style and
        // leaderLine; they are dropped so it can be fed back.
        protected override void PrepareSettings(
            JObject settings,
            ISet<string> notes)
        {
            if (settings["style"] is JObject style)
            {
                foreach (var key in ReadOnlyStyleKeys)
                {
                    style.Remove(key);
                }
            }

            if (settings["leaderLine"] is JObject leaderLine)
            {
                foreach (var key in ReadOnlyLeaderLineKeys)
                {
                    leaderLine.Remove(key);
                }
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.LabelElements;

        public override Guid ComponentGuid =>
            new Guid("5b8e2f47-93c1-4d6a-b0e8-7a1f3c9d2e64");
    }
}
