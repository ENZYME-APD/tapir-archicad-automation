using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetBuildingMaterialsComponent : AttributeDetailsJsonComponent
    {
        public override string CommandName => "GetBuildingMaterials";

        public GetBuildingMaterialsComponent()
            : base(
                "GetBuildingMaterials",
                "Get the details of the given Building Material attributes.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetBuildingMaterials;

        public override Guid ComponentGuid =>
            new Guid("b5601564-506b-4f93-becf-74dd99502976");
    }
}
