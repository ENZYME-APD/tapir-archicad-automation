using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateTextsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateTexts";

        public CreateTextsComponent()
            : base(
                "CreateTexts",
                "Create Text elements based on the given parameters.",
                GroupNames.ElementCreation,
                "textsData",
                new List<Field>
                {
                    new Field("Positions", "coordinate", FieldKind.Point3D, "Position of each text element.", required: true),
                    new Field("Texts", "text", FieldKind.Text, "Content of the text element.", required: true),
                    new Field("Heights", "height", FieldKind.Number, "Text height in millimeters."),
                    new Field("Angles", "angle", FieldKind.Number, "Rotation angle in radians."),
                    new Field("Justifications", "justification", FieldKind.Text, "Justification: Left, Center, Right or Full."),
                    new Field("Pens", "pen", FieldKind.Integer, "Pen index of the text."),
                    new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the text.")
                })
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateTexts;

        public override Guid ComponentGuid =>
            new Guid("ea685cf3-04f6-42c4-a363-4bdbba034ba4");
    }
}
