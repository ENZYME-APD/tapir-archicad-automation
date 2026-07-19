using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetLayersDetailsComponent : AttributeDetailsJsonComponent
    {
        public override string CommandName => "GetLayers";

        public GetLayersDetailsComponent()
            : base(
                "GetLayers",
                "Get the details of the given Layer attributes.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLayers;

        public override Guid ComponentGuid =>
            new Guid("fa0448c4-a557-4a87-8bb1-6efa6a948169");
    }
}
