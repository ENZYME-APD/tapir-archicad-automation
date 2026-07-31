using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyBeamsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyBeams";

        public ModifyBeamsComponent()
            : base(
                "ModifyBeams",
                "Modify Beam elements. Only the connected optional inputs are changed on the elements.",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "beamsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("BegPoints", "begCoordinate", FieldKind.Point2D, "New beginning point of the beam (only X and Y are used)."),
            new Field("EndPoints", "endCoordinate", FieldKind.Point2D, "New end point of the beam (only X and Y are used)."),
            new Field("Levels", "level", FieldKind.Number, "Base height of the beam relative to the home story."),
            new Field("Offsets", "offset", FieldKind.Number, "Offset of the beam from its reference line."),
            new Field("SlantAngles", "slantAngle", FieldKind.Number, "Slant angle of the beam in radians."),
            new Field("ArcAngles", "arcAngle", FieldKind.Number, "Arc angle in radians; a non-zero value makes the beam curved."),
            new Field("VerticalCurveHeights", "verticalCurveHeight", FieldKind.Number, "Height of the vertical curve of the beam.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyBeams;

        public override Guid ComponentGuid =>
            new Guid("537c6ad9-0c85-48ec-9544-54e0bbcca0e8");
    }
}
