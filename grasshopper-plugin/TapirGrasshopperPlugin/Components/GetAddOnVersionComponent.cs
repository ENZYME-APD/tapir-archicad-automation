using Grasshopper;
using Grasshopper.Kernel;
using Newtonsoft.Json;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Text.Json;
using TapirGrasshopperPlugin.Components;
using TapirGrasshopperPlugin.Utilities;

namespace Tapir.Components
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
                "Get Tapir Add-On version",
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

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetAddOnVersion", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            VersionInfo versionInfo = response.Result.ToObject<VersionInfo> ();
            DA.SetData (0, versionInfo.Version);
        }

        protected override System.Drawing.Bitmap Icon => Properties.Resources.TapirLogo;

        public override Guid ComponentGuid => new Guid ("de017e94-ea0e-4947-bbf1-7c7d60e5e016");
    }
}
