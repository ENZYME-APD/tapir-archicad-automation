using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetZoneCategoriesComponent : AttributeDetailsJsonComponent
    {
        public override string CommandName => "GetZoneCategories";

        public GetZoneCategoriesComponent()
            : base(
                "GetZoneCategories",
                "Get the details of the given Zone Category attributes.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetZoneCategories;

        public override Guid ComponentGuid =>
            new Guid("7886626e-e76e-4d2a-a84d-75e49c34fee3");
    }
}
