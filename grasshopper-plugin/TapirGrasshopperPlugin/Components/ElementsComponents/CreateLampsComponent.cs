using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateLampsComponent : LibraryPartCreatorComponent
    {
        public override string CommandName => "CreateLamps";

        public CreateLampsComponent()
            : base(
                "CreateLamps",
                "Create Lamp elements based on the given parameters.",
                "lampsData")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLamps;

        public override Guid ComponentGuid =>
            new Guid("100aa4b2-19a5-426c-b7e5-2ee55a03e9a6");
    }
}
