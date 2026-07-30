using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateFillsComponent : AttributeCreateJsonComponent
    {
        public override string CommandName => "CreateFills";

        public CreateFillsComponent()
            : base(
                "CreateFills",
                "Create or overwrite Fill attributes based on the given parameters.",
                "fillDataArray")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateFills;

        public override Guid ComponentGuid =>
            new Guid("d4132885-3480-4d68-9e2c-81cd5d73bbfd");
    }
}
