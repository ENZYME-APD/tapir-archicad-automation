using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateWallsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateWalls";

        public CreateWallsComponent()
            : base(
                "CreateWalls",
                "Create Wall elements based on the given parameters. " +
                "The Z coordinate of each line's start point is used as the wall's Z coordinate.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "wallsData";

        protected override bool HasAdditionalSettingsInput => false;

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Lines", null, FieldKind.Line, "Reference lines of the walls (start and end point; the start point's Z is used as the wall's Z coordinate).", required: true),
            new Field("Heights", "height", FieldKind.Number, "Height of the wall.", required: true),
            new Field("Thicknesses", "thickness", FieldKind.Number, "Thickness of the wall.", required: true),
            new Field("Offsets", "offset", FieldKind.Number, "Offset of the wall from its reference line."),
            new Field("ArcAngles", "arcAngle", FieldKind.Number, "Arc angle in radians; a non-zero value makes the wall curved."),
            new Field("ReferenceLineLocations", "referenceLineLocation", FieldKind.Text, "Reference line location: Outside, Center, Inside, CoreOutside, CoreCenter or CoreInside."),
            new Field("StructureTypes", "structureType", FieldKind.Text, "Structure type: Basic, Composite or Profile."),
            new Field("BuildingMaterialGuids", "buildingMaterialId", FieldKind.AttributeGuid, "Building material attribute for Basic structure."),
            new Field("CompositeGuids", "compositeId", FieldKind.AttributeGuid, "Composite attribute for Composite structure."),
            new Field("ProfileGuids", "profileId", FieldKind.AttributeGuid, "Profile attribute for Profile structure."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the wall.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateWalls;

        public override Guid ComponentGuid =>
            new Guid("2dd6d5d2-a759-40fb-b3b2-6469ea675b67");
    }
}
