using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetProfilesComponent : AttributeDetailsJsonComponent
    {
        public override string CommandName => "GetProfiles";

        public GetProfilesComponent()
            : base(
                "GetProfiles",
                "Get the details of the given Profile attributes.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetProfiles;

        public override Guid ComponentGuid =>
            new Guid("601e3fbf-cf26-4a6f-a1cf-f45fa38bbac0");
    }
}
