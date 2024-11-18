using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class VersionInfo
    {
        [JsonProperty ("version")]
        public string Version { get; set; }
    }

    public class GetAddOnVersionComponent : ArchicadAccessorComponent
    {
        public GetAddOnVersionComponent ()
          : base (
                "Tapir Version",
                "TapirVersion",
                "Get Tapir Add-On version.",
                "General"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter ("Version", "Version", "Tapir Add-On version.", GH_ParamAccess.item);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetAddOnVersion", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            VersionInfo versionInfo = response.Result.ToObject<VersionInfo> ();
            DA.SetData (0, versionInfo.Version);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.TapirVersion;

        public override Guid ComponentGuid => new Guid ("de017e94-ea0e-4947-bbf1-7c7d60e5e016");
    }
}
