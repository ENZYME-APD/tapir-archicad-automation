using System;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetIFCTypeOfElementsComponent : ElementsJsonAccessorComponent
    {
        public override string CommandName => "GetIFCTypeOfElements";

        public GetIFCTypeOfElementsComponent()
            : base(
                "GetIFCTypeOfElements",
                "Retrieve the IFC types of the given elements.",
                GroupNames.IFC,
                "IFCTypes")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetIFCTypeOfElements;

        public override Guid ComponentGuid =>
            new Guid("94fef72c-c0f0-40ac-9347-3c45ff81801f");
    }
}
