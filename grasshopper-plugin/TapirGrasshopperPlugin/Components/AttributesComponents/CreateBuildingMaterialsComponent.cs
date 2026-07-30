using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateBuildingMaterialsComponent : AttributeCreateJsonComponent
    {
        public override string CommandName => "CreateBuildingMaterials";

        public CreateBuildingMaterialsComponent()
            : base(
                "CreateBuildingMaterials",
                "Create or overwrite Building Material attributes based on the given parameters.",
                "buildingMaterialDataArray")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateBuildingMaterials;

        public override Guid ComponentGuid =>
            new Guid("dd6d2b7d-4900-4fbd-ac9c-222e9a3a3868");
    }
}
