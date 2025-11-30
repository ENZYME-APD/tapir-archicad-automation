using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetAddOnVersionComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAddOnVersion";

        public GetAddOnVersionComponent()
            : base(
                "TapirVersion",
                "Get Tapir Add-On version.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutText("TapirVersion");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedValues(
                    CommandName,
                    null,
                    SendToAddOn,
                    JHelp.Deserialize<VersionInfo>,
                    out VersionInfo response))
            {
                return;
            }

            da.SetData(
                0,
                response.Version);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.TapirVersion;

        public override Guid ComponentGuid =>
            new Guid("de017e94-ea0e-4947-bbf1-7c7d60e5e016");
    }
}