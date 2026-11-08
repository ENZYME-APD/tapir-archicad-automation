using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyRoofsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyRoofs";

        public ModifyRoofsComponent()
            : base(
                "ModifyRoofs",
                "Modify single-plane and multi-plane Roof elements. Only the connected optional inputs are changed on the elements. " +
                "The outline polygon and holes can be modified through the AdditionalSettings input (polygonOutline, polygonArcs, holes); " +
                "for single-plane roofs also angle and pivotLine (begCoordinate and endCoordinate), " +
                "for multi-plane roofs also levels and pivotEdges (the angle, eaves overhang and gable of each edge).",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "roofsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Levels", "level", FieldKind.Number, "Reference level of the roof."),
            new Field("Thicknesses", "thickness", FieldKind.Number, "Thickness of the roof."),
            new Field("EavesOverhangs", "eavesOverhang", FieldKind.Number, "Eaves overhang of a multi-plane roof."),
            new Field("StructureTypes", "structureType", FieldKind.Text, "Structure type: Basic or Composite.", valueList: () => new StructureTypeValueList ()),
            new Field("BuildingMaterialGuids", "buildingMaterialId", FieldKind.AttributeGuid, "Building material attribute for Basic structure."),
            new Field("CompositeGuids", "compositeId", FieldKind.AttributeGuid, "Composite attribute for Composite structure.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        // After the AdditionalSettings input, so saved definitions keep their inputs.
        private static readonly List<Field> TrailingFieldDefinitions = new List<Field>
        {
            new Field("Angles", "angle", FieldKind.Number, "Angle of a single-plane roof's plane to the XY plane, in radians.")
        };

        protected override IReadOnlyList<Field> TrailingFields => TrailingFieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyRoofs;

        public override Guid ComponentGuid =>
            new Guid("fdf98319-28ae-4ae0-9e7d-8a3810e4ff5e");
    }
}
