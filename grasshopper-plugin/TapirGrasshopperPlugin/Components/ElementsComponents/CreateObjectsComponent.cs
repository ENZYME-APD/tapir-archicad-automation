using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateObjectsComponent : LibraryPartCreatorComponent
    {
        public override string CommandName => "CreateObjects";

        public CreateObjectsComponent()
            : base(
                "CreateObjects",
                "Create Object elements based on the given parameters.",
                "objectsData")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateObjects;

        public override Guid ComponentGuid =>
            new Guid("4c005651-cd88-477b-ac6a-32f102c4f065");
    }
}
