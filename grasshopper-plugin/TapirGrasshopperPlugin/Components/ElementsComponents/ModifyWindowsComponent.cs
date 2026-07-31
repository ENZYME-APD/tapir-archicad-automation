using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyWindowsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyWindows";

        public ModifyWindowsComponent()
            : base(
                "ModifyWindows",
                "Modify Window elements. Only the connected optional inputs are changed on the elements.",
                GroupNames.ElementModification,
                "windowsWithDetails",
                new List<Field>
                {
                    new Field("Widths", "width", FieldKind.Number, "Width of the window."),
                    new Field("Heights", "height", FieldKind.Number, "Height of the window."),
                    new Field("SillHeights", "sillHeight", FieldKind.Number, "Sill height of the window."),
                    new Field("CenterOffsets", "centerOffset", FieldKind.Number, "Distance of the window center from the beginning point of the owner wall."),
                    new Field("Reflected", "reflected", FieldKind.Boolean, "Mirror the window on its vertical axis."),
                    new Field("RefSide", "refSide", FieldKind.Boolean, "Place the window on the reference line side of the owner wall."),
                    new Field("OSide", "oSide", FieldKind.Boolean, "Place the window on the side opposite to the reference line of the owner wall.")
                })
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyWindows;

        public override Guid ComponentGuid =>
            new Guid("45a03e3b-0ebd-4f98-a50b-aa6653878ca7");
    }
}
