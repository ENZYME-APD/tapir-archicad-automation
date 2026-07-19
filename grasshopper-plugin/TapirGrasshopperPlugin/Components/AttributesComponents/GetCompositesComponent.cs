using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetCompositesComponent : AttributeDetailsJsonComponent
    {
        public override string CommandName => "GetComposites";

        public GetCompositesComponent()
            : base(
                "GetComposites",
                "Get the details of the given Composite attributes.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetComposites;

        public override Guid ComponentGuid =>
            new Guid("a2c4d80f-81c1-47cd-9e04-2bf5f92eb7d7");
    }
}
