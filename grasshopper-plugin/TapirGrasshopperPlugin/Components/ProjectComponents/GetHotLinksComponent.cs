using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Properties;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetHotLinksComponent : ArchicadAccessorComponent
    {
        public GetHotLinksComponent ()
            : base (
                nameof(Hotlinks),
                nameof(Hotlinks),
                "Gets the file system locations (path) of the hotlink modules. The hotlinks can have tree hierarchy in the project.",
                "Project"
            )
        {}

        protected override void RegisterInputParams (GH_InputParamManager pManager){}

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(nameof(Hotlink.Location) + "s", "", "", GH_ParamAccess.list);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            var response = SendArchicadAddOnCommand ("TapirCommand", "GetHotlinks", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            var hResponse = response.Result.ToObject<GetHotlinksResponse> ();

            DA.SetDataList(0, hResponse.Hotlinks.GetLocationsRecursively());
        }

        protected override System.Drawing.Bitmap Icon => Properties.Resources.ProjectLocation;

        public override Guid ComponentGuid => new Guid ("a0b9722f-bc40-4ac3-afbc-d93e21dd8975");
    }
}
