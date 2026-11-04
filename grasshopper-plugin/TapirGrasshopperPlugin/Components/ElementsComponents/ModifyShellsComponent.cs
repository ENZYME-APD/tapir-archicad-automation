using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyShellsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyShells";

        public ModifyShellsComponent()
            : base(
                "ModifyShells",
                "Modify the structure of Shell elements: Basic with a building material or Composite with " +
                "a composite. Only the connected optional inputs are changed on the elements.",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "shellsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("StructureTypes", "structureType", FieldKind.Text, "Structure type: Basic or Composite.", valueList: () => new StructureTypeValueList ()),
            new Field("BuildingMaterialGuids", "buildingMaterialId", FieldKind.AttributeGuid, "Building material attribute for Basic structure."),
            new Field("CompositeGuids", "compositeId", FieldKind.AttributeGuid, "Composite attribute for Composite structure.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        // Shares the icon of ModifyRoofs until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyRoofs;

        public override Guid ComponentGuid =>
            new Guid("83bb70a5-fc7c-45f4-ad4d-c0c0f58ea249");
    }
}
