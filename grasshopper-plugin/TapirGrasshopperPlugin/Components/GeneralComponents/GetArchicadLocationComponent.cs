using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetArchicadLocationComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetArchicadLocation";

        public GetArchicadLocationComponent()
            : base(
                "ArchicadLocation",
                "Get the location of the running Archicad executable.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "Location",
                "Location of the running Archicad executable.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedResponse(
                    CommandName,
                    out LocationInfo response))
            {
                return;
            }

            da.SetData(
                0,
                response.ArchicadLocation);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ArchicadLocation;

        public override Guid ComponentGuid =>
            new Guid("8863e688-7b90-47df-918f-f7a8f27bfa54");
    }
}