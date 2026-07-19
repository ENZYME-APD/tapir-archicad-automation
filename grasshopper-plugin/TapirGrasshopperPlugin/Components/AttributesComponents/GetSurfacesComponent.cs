using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetSurfacesComponent : AttributeDetailsJsonComponent
    {
        public override string CommandName => "GetSurfaces";

        public GetSurfacesComponent()
            : base(
                "GetSurfaces",
                "Get the details of the given Surface attributes.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetSurfaces;

        public override Guid ComponentGuid =>
            new Guid("1da2e5de-3f08-4592-88b3-4c72ee479414");
    }
}
