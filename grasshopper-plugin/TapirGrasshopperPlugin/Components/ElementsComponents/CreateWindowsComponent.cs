using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateWindowsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateWindows";

        public CreateWindowsComponent()
            : base(
                "CreateWindows",
                "Create Window elements in the given owner walls.",
                GroupNames.ElementCreation,
                "windowsData",
                new List<Field>
                {
                    new Field("OwnerWallGuids", "ownerWallId", FieldKind.ElementGuid, "Identifiers of the walls to place the windows into.", required: true),
                    new Field("CenterOffsets", "centerOffset", FieldKind.Number, "Distance of the window center from the beginning point of the owner wall.", required: true),
                    new Field("SillHeights", "sillHeight", FieldKind.Number, "Sill height of the window."),
                    new Field("Widths", "width", FieldKind.Number, "Width of the window."),
                    new Field("Heights", "height", FieldKind.Number, "Height of the window."),
                    new Field("Reflected", "reflected", FieldKind.Boolean, "Mirror the window on its vertical axis."),
                    new Field("RefSide", "refSide", FieldKind.Boolean, "Place the window on the reference line side of the owner wall."),
                    new Field("OSide", "oSide", FieldKind.Boolean, "Place the window on the side opposite to the reference line of the owner wall."),
                    new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of the favorite to use as a base for the window.")
                })
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateWindows;

        public override Guid ComponentGuid =>
            new Guid("b981af9d-167a-45ce-a7ec-143bc5e53970");
    }
}
