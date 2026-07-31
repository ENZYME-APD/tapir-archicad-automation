using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateRoofsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateRoofs";

        public CreateRoofsComponent()
            : base(
                "CreateRoofs",
                "Create multi-plane Roof elements based on the given parameters. " +
                "Holes, arcs, roof levels and single-plane roofs (pivotLine, angle) can be given " +
                "through the AdditionalSettings input.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "roofsData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Polygons", "polygonCoordinates", FieldKind.PointsTree2D, "Outline points of each roof (one branch per roof, at least 3 points; only X and Y are used).", required: true, minPointsPerBranch: 3),
            new Field("Levels", "level", FieldKind.Number, "Reference level of the roof.", required: true),
            new Field("Thicknesses", "thickness", FieldKind.Number, "Thickness of the roof."),
            new Field("EavesOverhangs", "eavesOverhang", FieldKind.Number, "Eaves overhang of the roof."),
            new Field("StructureTypes", "structureType", FieldKind.Text, "Structure type: Basic or Composite."),
            new Field("BuildingMaterialGuids", "buildingMaterialId", FieldKind.AttributeGuid, "Building material attribute for Basic structure."),
            new Field("CompositeGuids", "compositeId", FieldKind.AttributeGuid, "Composite attribute for Composite structure."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the roof.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateRoofs;

        public override Guid ComponentGuid =>
            new Guid("7bff9b96-91ba-434b-b052-fa88c2744ce6");
    }
}
