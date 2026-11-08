using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateObjectsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateObjects";

        public CreateObjectsComponent()
            : base(
                "CreateObjects",
                "Place Object elements from the loaded libraries. " +
                "Dimensions, pens, attributes and visibility settings can be given through the " +
                "AdditionalSettings input (dimensions, pen, surfaceId, visibility, etc.).",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "objectsData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Points", "coordinates", FieldKind.Point3D, "Insertion points of the objects.", required: true),
            new Field("LibraryPartNames", "libraryPartName", FieldKind.Text, "Name of the library part to place.", required: true),
            new Field("Angles", "angle", FieldKind.Number, "Rotation angle in radians."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the object."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        // Appended after the component was released (#2018), so saved definitions keep their inputs.
        private static readonly List<Field> TrailingFieldDefinitions = new List<Field>
        {
            new Field("DatabaseId", "databaseId", FieldKind.DatabaseId, "Database to place the objects in, e.g. the DatabaseId of CreateWorksheets or GetDatabases. Optional, default is the current database.")
        };

        protected override IReadOnlyList<Field> TrailingFields => TrailingFieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateObjects;

        public override Guid ComponentGuid =>
            new Guid("4c005651-cd88-477b-ac6a-32f102c4f065");
    }
}
