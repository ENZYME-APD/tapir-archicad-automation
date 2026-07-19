using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateSurfacesComponent : AttributeCreateJsonComponent
    {
        public override string CommandName => "CreateSurfaces";

        public CreateSurfacesComponent()
            : base(
                "CreateSurfaces",
                "Create or overwrite Surface attributes based on the given parameters.",
                "surfaceDataArray")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSurfaces;

        public override Guid ComponentGuid =>
            new Guid("6e1b7d48-40ad-4c62-94c0-ae04094b57b7");
    }
}
