using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetMEPSystemsComponent : AttributeDetailsJsonComponent
    {
        public override string CommandName => "GetMEPSystems";

        public GetMEPSystemsComponent()
            : base(
                "GetMEPSystems",
                "Get the details of the given MEP System attributes.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPSystems;

        public override Guid ComponentGuid =>
            new Guid("7b0e3bed-9ebd-45b3-8239-6f77686a56ca");
    }
}
