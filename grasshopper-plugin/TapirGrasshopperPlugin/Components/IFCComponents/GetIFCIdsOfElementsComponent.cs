using System;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetIFCIdsOfElementsComponent : ElementsJsonAccessorComponent
    {
        public override string CommandName => "GetIFCIdsOfElements";

        public GetIFCIdsOfElementsComponent()
            : base(
                "GetIFCIdsOfElements",
                "Retrieve the IFC identifiers of the given elements.",
                GroupNames.IFC,
                "IFCIds")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetIFCIdsOfElements;

        public override Guid ComponentGuid =>
            new Guid("c81c04eb-8f87-417e-b1e6-8008f8ee5db6");
    }
}
