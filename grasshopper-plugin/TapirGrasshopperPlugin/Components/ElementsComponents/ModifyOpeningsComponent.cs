using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyOpeningsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyOpenings";

        public ModifyOpeningsComponent()
            : base(
                "ModifyOpenings",
                "Modify Opening elements. Only the connected optional inputs are changed on the elements.",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "openingsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("BasePoints", "basePoint", FieldKind.Point3D, "New base point of the opening: its bottom left corner in world space, the same point CreateOpenings takes."),
            new Field("Widths", "width", FieldKind.Number, "New width of the opening."),
            new Field("Heights", "height", FieldKind.Number, "New height of the opening. The bottom edge stays where it is.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        // Shares the Modify Windows icon until an own one is drawn, the way
        // CreateLayers shares the SetLayers icon.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyWindows;

        public override Guid ComponentGuid =>
            new Guid("ee673115-c6ac-4158-8e38-648f34cee177");
    }
}
