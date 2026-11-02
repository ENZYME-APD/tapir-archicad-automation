using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyCurtainWallsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyCurtainWalls";

        public ModifyCurtainWallsComponent()
            : base(
                "ModifyCurtainWalls",
                "Modify Curtain Wall elements in place, keeping their GUID, properties and classifications. " +
                "Only the connected optional inputs are changed on the elements. The reference line " +
                "(coordinates, with the same number of segments), the column and row pattern (primaryPattern, " +
                "secondaryPattern) and the cell assignments (cells) can be modified through the " +
                "AdditionalSettings input, in the fields of CreateCurtainWalls.",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "curtainWallsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Heights", "height", FieldKind.Number, "Height of the curtain wall."),
            new Field("ZCoordinates", "zCoordinate", FieldKind.Number, "Z coordinate (absolute elevation) of the bottom of the curtain wall. The home story is derived from it unless FloorIndices is given too."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the curtain wall. On its own it moves the curtain wall to that story, keeping its bottom offset from the story.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        // No ModifyCurtainWalls icon exists yet (a curtain wall with the pencil badge of the
        // other Modify icons); until one is added to Resources, the CreateCurtainWalls icon
        // marks the component as the curtain wall one.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateCurtainWalls;

        public override Guid ComponentGuid =>
            new Guid("3c2f6b7e-9d41-4a8b-b5e2-7f0c1d9a6e54");
    }
}
