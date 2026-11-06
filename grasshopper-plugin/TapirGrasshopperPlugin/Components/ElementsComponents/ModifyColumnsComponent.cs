using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyColumnsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyColumns";

        public ModifyColumnsComponent()
            : base(
                "ModifyColumns",
                "Modify Column elements. Only the connected optional inputs are changed on the elements.",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "columnsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Origins", "origin", FieldKind.Point2D, "New origin of the column (only X and Y are used)."),
            new Field("ZCoordinates", "zCoordinate", FieldKind.Number, "Bottom level of the column."),
            new Field("Heights", "height", FieldKind.Number, "Height of the column."),
            new Field("BottomOffsets", "bottomOffset", FieldKind.Number, "Vertical offset of the column bottom from the home story level."),
            new Field("AxisRotationAngles", "axisRotationAngle", FieldKind.Number, "Rotation angle of the column axis in radians.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        private static readonly List<Field> TrailingFieldDefinitions = new List<Field>
        {
            new Field("Widths", "width", FieldKind.Number, "Cross section width of the column (applied to all segments)."),
            new Field("Depths", "depth", FieldKind.Number, "Cross section depth of the column (applied to all segments)."),
            new Field("CoreAnchors", "coreAnchor", FieldKind.Text, "Core anchor: TopLeft, TopCenter, TopRight, MiddleLeft, Center, MiddleRight, BottomLeft, BottomCenter or BottomRight."),
            new Field("CircleBased", "circleBased", FieldKind.Boolean, "True for a round column cross section, false for rectangular (applied to all segments)."),
            new Field("BuildingMaterialGuids", "buildingMaterialId", FieldKind.AttributeGuid, "Building material attribute of the round or rectangular cross section (applied to all segments)."),
            new Field("ProfileGuids", "profileId", FieldKind.AttributeGuid, "Profile attribute; switches the cross section to this custom profile (applied to all segments).")
        };

        protected override IReadOnlyList<Field> TrailingFields => TrailingFieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyColumns;

        public override Guid ComponentGuid =>
            new Guid("84cd940f-67e4-4b41-ad3e-b6a7af0eafa7");
    }
}
