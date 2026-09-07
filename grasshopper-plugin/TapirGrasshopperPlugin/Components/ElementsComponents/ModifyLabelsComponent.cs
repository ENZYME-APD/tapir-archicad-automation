using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyLabelsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyLabels";

        public ModifyLabelsComponent()
            : base(
                "ModifyLabels",
                "Modify Label elements. Only the connected optional inputs are changed on the elements. " +
                "The text inputs apply to text labels only; a change of the text or of a text style input " +
                "rebuilds the content as one paragraph, which makes the label auto-width. The leader line " +
                "inputs apply to both label classes. Multi-run content, effects, the arrow pen, the anchor " +
                "point and the symbol label settings can be changed through the AdditionalSettings input " +
                "(runs, style.effectStrikeout, leaderLine.arrowPenIndex, leaderLine.anchorPoint, symbolStyle, etc.).",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "labelsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Texts", "text", FieldKind.Text, "New content of the text label. Newlines create multiple lines."),
            new Field("Heights", "style.height", FieldKind.Number, "Text height in millimeters."),
            new Field("Pens", "style.penIndex", FieldKind.Integer, "Pen index of the text."),
            new Field("Fonts", "style.fontIndex", FieldKind.Integer, "Font attribute index of the text."),
            new Field("Bold", "style.bold", FieldKind.Boolean, "Bold face."),
            new Field("Italic", "style.italic", FieldKind.Boolean, "Italic face."),
            new Field("Underline", "style.underline", FieldKind.Boolean, "Underlined face."),
            new Field("LeaderPens", "leaderLine.penIndex", FieldKind.Integer, "Pen index of the leader line."),
            new Field("LeaderLineTypeGuids", "leaderLine.lineTypeId", FieldKind.AttributeGuid, "Line type attribute of the leader line."),
            new Field("HasLeaderLines", "leaderLine.hasLeaderLine", FieldKind.Boolean, "Whether the label has a leader line."),
            new Field("Framed", "leaderLine.framed", FieldKind.Boolean, "Put a frame around the content."),
            new Field("LeaderShapes", "leaderLine.leaderShape", FieldKind.Text, "Shape of the leader line: Segmented, Splinear or SquareRoot."),
            new Field("ArrowTypes", "leaderLine.arrowType", FieldKind.Text, "Arrow head shape of the leader line, e.g. FullArrow30, EmptyCircle or SlashLine45."),
            new Field("ArrowVisible", "leaderLine.arrowVisible", FieldKind.Boolean, "Whether the arrow head is drawn."),
            new Field("ArrowSizes", "leaderLine.arrowSize", FieldKind.Number, "Arrow size in millimeters.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyLabels;

        public override Guid ComponentGuid =>
            new Guid("a4c385cb-bc83-4f5c-9b96-77c22d3e0427");
    }
}
