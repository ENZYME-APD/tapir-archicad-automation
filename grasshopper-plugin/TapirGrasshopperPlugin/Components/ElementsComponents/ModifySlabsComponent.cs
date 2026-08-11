using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifySlabsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifySlabs";

        public ModifySlabsComponent()
            : base(
                "ModifySlabs",
                "Modify Slab elements. Only the connected optional inputs are changed on the elements. " +
                "The outline polygon and holes can be modified through the AdditionalSettings input " +
                "(polygonOutline, polygonArcs, holes).",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "slabsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("ZCoordinates", "zCoordinate", FieldKind.Number, "Reference level of the slab."),
            new Field("Thicknesses", "thickness", FieldKind.Number, "Thickness of the slab."),
            new Field("StructureTypes", "structureType", FieldKind.Text, "Structure type: Basic or Composite.", valueList: () => new StructureTypeValueList ()),
            new Field("BuildingMaterialGuids", "buildingMaterialId", FieldKind.AttributeGuid, "Building material attribute for Basic structure."),
            new Field("CompositeGuids", "compositeId", FieldKind.AttributeGuid, "Composite attribute for Composite structure.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifySlabs;

        public override Guid ComponentGuid =>
            new Guid("7d7adda2-6afd-4371-8f0d-854e82b65dd7");
    }
}
