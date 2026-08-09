using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateBeamsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateBeams";

        public CreateBeamsComponent()
            : base(
                "CreateBeams",
                "Create Beam elements based on the given parameters. " +
                "The Z coordinate of each line's start point is used as the beam's Z coordinate.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "beamsData";

        protected override bool HasAdditionalSettingsInput => false;

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Lines", null, FieldKind.Line, "Reference lines of the beams (start and end point; the start point's Z is used as the beam's Z coordinate).", required: true),
            new Field("Widths", "width", FieldKind.Number, "Width of the beam."),
            new Field("Heights", "height", FieldKind.Number, "Height of the beam."),
            new Field("Offsets", "offset", FieldKind.Number, "Offset of the beam from its reference line."),
            new Field("SlantAngles", "slantAngle", FieldKind.Number, "Slant angle of the beam in radians."),
            new Field("ArcAngles", "arcAngle", FieldKind.Number, "Arc angle in radians; a non-zero value makes the beam curved."),
            new Field("VerticalCurveHeights", "verticalCurveHeight", FieldKind.Number, "Height of the vertical curve of the beam."),
            new Field("AnchorPoints", "anchorPoint", FieldKind.Text, "Anchor point: TopLeft, TopCenter, TopRight, MiddleLeft, Center, MiddleRight, BottomLeft, BottomCenter or BottomRight."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the beam."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateBeams;

        public override Guid ComponentGuid =>
            new Guid("257a44ff-33c2-43c0-9fca-e587cfddec78");
    }
}
