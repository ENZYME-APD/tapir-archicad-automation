using System;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetLayoutCustomSchemeComponent : NoInputJsonAccessorComponent
    {
        public override string CommandName => "GetLayoutCustomScheme";

        public GetLayoutCustomSchemeComponent()
            : base(
                "GetLayoutCustomScheme",
                "Get the Layout Info Panel custom field definitions (name and key) from Book Settings.",
                GroupNames.Navigator,
                "CustomScheme")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLayoutCustomScheme;

        public override Guid ComponentGuid =>
            new Guid("fb8d59b6-15b1-4f89-9e6c-64cb07d0ef3f");
    }
}
