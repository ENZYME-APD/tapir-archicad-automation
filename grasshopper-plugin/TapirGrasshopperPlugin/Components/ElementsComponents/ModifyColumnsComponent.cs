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

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyColumns;

        public override Guid ComponentGuid =>
            new Guid("84cd940f-67e4-4b41-ad3e-b6a7af0eafa7");
    }
}
