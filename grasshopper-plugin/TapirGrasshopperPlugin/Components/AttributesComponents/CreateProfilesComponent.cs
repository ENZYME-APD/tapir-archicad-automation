using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateProfilesComponent : AttributeCreateJsonComponent
    {
        public override string CommandName => "CreateProfiles";

        public CreateProfilesComponent()
            : base(
                "CreateProfiles",
                "Create or overwrite Profile attributes as a copy of an existing Profile's geometry, based on the given parameters.",
                "profileDataArray")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateProfiles;

        public override Guid ComponentGuid =>
            new Guid("dbaa0635-ca5d-46dc-9a59-6f3780fbca63");
    }
}
