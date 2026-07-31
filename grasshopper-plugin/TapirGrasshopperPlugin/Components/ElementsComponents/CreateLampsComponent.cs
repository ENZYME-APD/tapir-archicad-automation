using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateLampsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateLamps";

        public CreateLampsComponent()
            : base(
                "CreateLamps",
                "Place Lamp elements from the loaded libraries. " +
                "Dimensions, pens, attributes, visibility and light settings can be given through the " +
                "AdditionalSettings input (dimensions, pen, surfaceId, visibility, lightIsOn, etc.).",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "lampsData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Points", "coordinates", FieldKind.Point3D, "Insertion points of the lamps.", required: true),
            new Field("LibraryPartNames", "libraryPartName", FieldKind.Text, "Name of the library part to place.", required: true),
            new Field("Angles", "angle", FieldKind.Number, "Rotation angle in radians."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the lamp.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLamps;

        public override Guid ComponentGuid =>
            new Guid("100aa4b2-19a5-426c-b7e5-2ee55a03e9a6");
    }
}
