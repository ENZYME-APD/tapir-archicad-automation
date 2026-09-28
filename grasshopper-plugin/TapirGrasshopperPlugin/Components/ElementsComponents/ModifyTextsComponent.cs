using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyTextsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyTexts";

        public ModifyTextsComponent()
            : base(
                "ModifyTexts",
                "Modify standalone Text elements. Only the connected optional inputs are changed on the " +
                "elements. A change of the text, or of the pen, font, face or height, rebuilds the content as " +
                "one paragraph, which makes the element auto-width; the other inputs leave the content as it is. Multi-run content (runs), effects, the text frame and " +
                "the anchor can be changed through the AdditionalSettings input (runs, style.effectStrikeout, " +
                "style.textFrameShape, style.anchor, etc.).",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "textsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Positions", "coordinate", FieldKind.Point3D, "New position of the text; its Z selects the floor when FloorIndices is not given."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the text."),
            new Field("Texts", "text", FieldKind.Text, "New content of the text. Newlines create multiple lines."),
            new Field("Heights", "style.height", FieldKind.Number, "Text height in millimeters."),
            new Field("Angles", "style.angle", FieldKind.Number, "Rotation angle in radians."),
            new Field("Justifications", "style.justification", FieldKind.Text, "Justification: Left, Center, Right or Full.", valueList: () => new TextJustificationValueList ()),
            new Field("Pens", "style.penIndex", FieldKind.Integer, "Pen index of the text."),
            new Field("Fonts", "style.fontIndex", FieldKind.Integer, "Font attribute index of the text."),
            new Field("Bold", "style.bold", FieldKind.Boolean, "Bold face."),
            new Field("Italic", "style.italic", FieldKind.Boolean, "Italic face."),
            new Field("Underline", "style.underline", FieldKind.Boolean, "Underlined face.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyTexts;

        public override Guid ComponentGuid =>
            new Guid("dfd9c59f-7089-4f6a-805d-15c974243de9");
    }
}
