using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateMEPSystemsComponent : AttributeCreateJsonComponent
    {
        public override string CommandName => "CreateMEPSystems";

        public CreateMEPSystemsComponent()
            : base(
                "CreateMEPSystems",
                "Create or overwrite MEP System attributes based on the given parameters.",
                "mepSystemDataArray")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMEPSystems;

        public override Guid ComponentGuid =>
            new Guid("3467ac36-4387-461f-a131-ec612d686b18");
    }
}
