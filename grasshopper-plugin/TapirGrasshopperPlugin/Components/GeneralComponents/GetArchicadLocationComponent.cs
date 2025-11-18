using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class LocationInfo
    {
        [JsonProperty("archicadLocation")]
        public string ArchicadLocation { get; set; }
    }

    public class GetArchicadLocationComponent : ArchicadAccessorComponent
    {
        public static string Doc =>
            "Get the location of the running Archicad executable.";

        public override string CommandName => "GetArchicadLocation";

        public GetArchicadLocationComponent()
            : base(
                "Archicad Location",
                "ArchicadLocation",
                Doc,
                GroupNames.General)
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
            IGH_DataAccess da)
        {
            if (!GetConvertedResponse(
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