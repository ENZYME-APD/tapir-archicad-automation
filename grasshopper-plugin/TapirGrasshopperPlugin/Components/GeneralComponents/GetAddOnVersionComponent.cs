using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class VersionInfo
    {
        [JsonProperty("version")]
        public string Version { get; set; }
    }

    public class GetAddOnVersionComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Get Tapir Add-On version.";
        public override string CommandName => "GetAddOnVersion";

        public GetAddOnVersionComponent()
            : base(
                "Tapir Version",
                "Tapir Version",
                Doc,
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "Version",
                "Tapir Add-On version.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!GetConvertedResponse(
                    CommandName,
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