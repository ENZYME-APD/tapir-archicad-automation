using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetFillsComponent : AttributeDetailsJsonComponent
    {
        public override string CommandName => "GetFills";

        public GetFillsComponent()
            : base(
                "GetFills",
                "Get the details of the given Fill attributes.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetFills;

        public override Guid ComponentGuid =>
            new Guid("37878650-64df-4df7-8818-d321a899956f");
    }
}
