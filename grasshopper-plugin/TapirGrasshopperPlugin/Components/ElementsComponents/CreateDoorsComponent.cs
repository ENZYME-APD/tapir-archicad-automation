using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateDoorsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateDoors";

        public CreateDoorsComponent()
            : base(
                "CreateDoors",
                "Create Door elements in the given owner walls.",
                GroupNames.ElementCreation,
                "doorsData",
                new List<Field>
                {
                    new Field("OwnerWallGuids", "ownerWallId", FieldKind.ElementGuid, "Identifiers of the walls to place the doors into.", required: true),
                    new Field("CenterOffsets", "centerOffset", FieldKind.Number, "Distance of the door center from the beginning point of the owner wall.", required: true),
                    new Field("SillHeights", "sillHeight", FieldKind.Number, "Sill height of the door."),
                    new Field("Widths", "width", FieldKind.Number, "Width of the door."),
                    new Field("Heights", "height", FieldKind.Number, "Height of the door."),
                    new Field("Reflected", "reflected", FieldKind.Boolean, "Mirror the door on its vertical axis."),
                    new Field("RefSide", "refSide", FieldKind.Boolean, "Place the door on the reference line side of the owner wall."),
                    new Field("OSide", "oSide", FieldKind.Boolean, "Place the door on the side opposite to the reference line of the owner wall."),
                    new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of the favorite to use as a base for the door.")
                })
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateDoors;

        public override Guid ComponentGuid =>
            new Guid("7eadb251-9dac-496d-98a6-8bba8e41e6c6");
    }
}
