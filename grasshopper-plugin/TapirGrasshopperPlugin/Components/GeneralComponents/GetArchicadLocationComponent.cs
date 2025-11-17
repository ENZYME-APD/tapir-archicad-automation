using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class LocationInfo
    {
        [JsonProperty("archicadLocation")]
        public string ArchicadLocation { get; set; }
    }

    public class GetArchicadLocationComponent : ArchicadAccessorComponent
    {
        public GetArchicadLocationComponent()
            : base(
                "Archicad Location",
                "ArchicadLocation",
                "Get the location of the running Archicad executable.",
                "General")
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Location",
                "Location",
                "Location of the running Archicad executable.",
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess DA)
        {
            var response = SendArchicadAddOnCommand(
                "GetArchicadLocation",
                null);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return;
            }

            var locationInfo = response.Result.ToObject<LocationInfo>();
            DA.SetData(
                0,
                locationInfo.ArchicadLocation);
        }

        protected override System.Drawing.Bitmap Icon =>
            TapirGrasshopperPlugin.Properties.Resources.ArchicadLocation;

        public override Guid ComponentGuid =>
            new Guid("8863e688-7b90-47df-918f-f7a8f27bfa54");
    }
}