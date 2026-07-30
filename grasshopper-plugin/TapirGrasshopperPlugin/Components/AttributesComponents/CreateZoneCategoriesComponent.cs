using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateZoneCategoriesComponent : AttributeCreateJsonComponent
    {
        public override string CommandName => "CreateZoneCategories";

        public CreateZoneCategoriesComponent()
            : base(
                "CreateZoneCategories",
                "Create or overwrite Zone Category attributes based on the given parameters.",
                "zoneCategoryDataArray")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateZoneCategories;

        public override Guid ComponentGuid =>
            new Guid("b82772cb-5a97-4651-b810-685e07de4b4f");
    }
}
