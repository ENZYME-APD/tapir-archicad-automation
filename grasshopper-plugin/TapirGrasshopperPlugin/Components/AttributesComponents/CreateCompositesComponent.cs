using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateCompositesComponent : AttributeCreateJsonComponent
    {
        public override string CommandName => "CreateComposites";

        public CreateCompositesComponent()
            : base(
                "CreateComposites",
                "Create or overwrite Composite attributes based on the given parameters.",
                "compositeDataArray")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateComposites;

        public override Guid ComponentGuid =>
            new Guid("e0990c58-6132-4bf8-84d2-0cdfaf8afe0e");
    }
}
