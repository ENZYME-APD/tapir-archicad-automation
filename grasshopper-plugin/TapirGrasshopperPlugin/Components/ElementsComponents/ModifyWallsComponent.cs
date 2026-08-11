using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyWallsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyWalls";

        public ModifyWallsComponent()
            : base(
                "ModifyWalls",
                "Modify Wall elements. Only the connected optional inputs are changed on the elements.",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "wallsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("BegPoints", "begCoordinate", FieldKind.Point2D, "New beginning point of the wall (only X and Y are used)."),
            new Field("EndPoints", "endCoordinate", FieldKind.Point2D, "New end point of the wall (only X and Y are used)."),
            new Field("ArcAngles", "arcAngle", FieldKind.Number, "Arc angle in radians; a non-zero value makes the wall curved."),
            new Field("Heights", "height", FieldKind.Number, "Height of the wall."),
            new Field("Thicknesses", "thickness", FieldKind.Number, "Thickness of the wall."),
            new Field("BottomOffsets", "bottomOffset", FieldKind.Number, "Vertical offset of the wall bottom from the home story level."),
            new Field("Offsets", "offset", FieldKind.Number, "Offset of the wall from its reference line."),
            new Field("StructureTypes", "structureType", FieldKind.Text, "Structure type: Basic, Composite or Profile.", valueList: () => new StructureTypeValueList ()),
            new Field("BuildingMaterialGuids", "buildingMaterialId", FieldKind.AttributeGuid, "Building material attribute for Basic structure."),
            new Field("CompositeGuids", "compositeId", FieldKind.AttributeGuid, "Composite attribute for Composite structure."),
            new Field("ProfileGuids", "profileId", FieldKind.AttributeGuid, "Profile attribute for Profile structure.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyWalls;

        public override Guid ComponentGuid =>
            new Guid("e7f0fec3-1341-4535-bfa4-31ae03978c93");
    }
}
