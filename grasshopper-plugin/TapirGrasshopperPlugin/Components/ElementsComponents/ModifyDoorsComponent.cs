using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyDoorsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyDoors";

        public ModifyDoorsComponent()
            : base(
                "ModifyDoors",
                "Modify Door elements. Only the connected optional inputs are changed on the elements.",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "doorsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Widths", "width", FieldKind.Number, "Width of the door."),
            new Field("Heights", "height", FieldKind.Number, "Height of the door."),
            new Field("SillHeights", "sillHeight", FieldKind.Number, "Sill height of the door."),
            new Field("CenterOffsets", "centerOffset", FieldKind.Number, "Distance of the door center from the beginning point of the owner wall."),
            new Field("Reflected", "reflected", FieldKind.Boolean, "Mirror the door on its vertical axis."),
            new Field("RefSide", "refSide", FieldKind.Boolean, "Place the door on the reference line side of the owner wall."),
            new Field("OSide", "oSide", FieldKind.Boolean, "Place the door on the side opposite to the reference line of the owner wall.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyDoors;

        public override Guid ComponentGuid =>
            new Guid("c1b56fd7-26c6-4d9a-9902-5280aec836ae");
    }
}
