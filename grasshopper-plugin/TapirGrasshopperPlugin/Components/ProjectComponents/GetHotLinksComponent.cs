using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetHotLinksComponent : ArchicadAccessorComponent
    {
        public static string CommandName => "GetHotlinks";

        public GetHotLinksComponent ()
            : base ( nameof (Hotlinks), nameof (Hotlinks), HotlinksResponse.Doc, GroupNames.Project )
        { }

        protected override void RegisterInputParams (GH_InputParamManager pManager){}

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(nameof(Hotlink.Location) + "List", "", "", GH_ParamAccess.list);
            pManager.AddTextParameter("JsonHierarchy", "", "", GH_ParamAccess.item);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            if (!GetResponse(CommandName, null, out HotlinksResponse response)) return;

            DA.SetDataList(0, response.Hotlinks.GetLocations());
            DA.SetData(1, JsonConvert.SerializeObject(response, Formatting.Indented));
        }

        public override Guid ComponentGuid => new Guid ("a0b9722f-bc40-4ac3-afbc-d93e21dd8975");
    }
}
