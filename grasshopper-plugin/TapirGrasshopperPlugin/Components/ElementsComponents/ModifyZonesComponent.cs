using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.ToolSettings;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // The inputs of the Zone settings are named after the labels of the Zone
    // Settings dialog (ToolSettings/Zone.json); a setting without a label
    // there keeps its API name.
    public class ModifyZonesComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyZones";

        public ModifyZonesComponent()
            : base(
                "ModifyZones",
                "Modify Zone elements: name, number, category, zone stamp, height, base offset, top link and " +
                "floor thickness. Only the connected optional inputs are changed on the elements; the " +
                "polygon is kept.",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "zonesWithDetails";

        private static string ZoneLabel(
            string key,
            string nameWithoutLabel)
        {
            return ToolSettingsTable.LabelOr(
                "Zone",
                key,
                nameWithoutLabel);
        }

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field(ZoneLabel("name", "Name"), "name", FieldKind.Text, "Name of the zone."),
            new Field(ZoneLabel("numberStr", "NumberStr"), "numberStr", FieldKind.Text, "Zone number."),
            new Field(ZoneLabel("categoryAttributeId", "CategoryAttributeGuid"), "categoryAttributeId", FieldKind.AttributeGuid, "The zone category attribute."),
            new Field(ZoneLabel("stampPosition", "StampPosition"), "stampPosition", FieldKind.Point2D, "Position of the origin of the zone stamp (only X and Y are used)."),
            new Field(ZoneLabel("stampAngle", "StampAngle"), "stampAngle", FieldKind.Number, "Rotation angle of the zone stamp in radians."),
            new Field(ZoneLabel("fixedStampAngle", "FixedStampAngle"), "fixedStampAngle", FieldKind.Boolean, "If true, the zone stamp angle remains fixed when the element is rotated."),
            new Field(ZoneLabel("height", "Height"), "height", FieldKind.Number, "Height of the zone from its bottom. Sets the top link to 0 (explicit height), so do not combine it with RelativeTopStory: whichever is applied last wins."),
            new Field(ZoneLabel("bottomOffset", "BottomOffset"), "bottomOffset", FieldKind.Number, "Base level of the zone relative to its home story level."),
            new Field(ZoneLabel("topOffset", "TopOffset"), "topOffset", FieldKind.Number, "Offset of the top of the zone from the linked top story level. Only has an effect when the top is linked to a story (RelativeTopStory is not 0)."),
            new Field(ZoneLabel("relativeTopStory", "RelativeTopStory"), "relativeTopStory", FieldKind.Integer, "Non-zero links the top of the zone to another story, relative to its home story (1 is the story above), instead of an explicit height. Do not combine it with Height."),
            new Field(ZoneLabel("floorThickness", "FloorThickness"), "floorThickness", FieldKind.Number, "Thickness of the floor finish (subfloor thickness) of the zone.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        // Shares the icon of UpdateZones until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UpdateZones;

        public override Guid ComponentGuid =>
            new Guid("4ffec0da-a093-4dcb-910b-01737d160522");
    }
}
