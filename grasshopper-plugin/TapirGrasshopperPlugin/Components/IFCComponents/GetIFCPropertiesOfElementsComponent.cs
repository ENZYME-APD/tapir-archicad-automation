using System;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetIFCPropertiesOfElementsComponent : ElementsJsonAccessorComponent
    {
        public override string CommandName => "GetIFCPropertiesOfElements";

        public GetIFCPropertiesOfElementsComponent()
            : base(
                "GetIFCPropertiesOfElements",
                "Retrieve the IFC properties of the given elements.",
                GroupNames.IFC,
                "IFCProperties")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetIFCPropertiesOfElements;

        public override Guid ComponentGuid =>
            new Guid("5da0e87b-0ec0-44c9-8d66-c377ad46d437");
    }
}
