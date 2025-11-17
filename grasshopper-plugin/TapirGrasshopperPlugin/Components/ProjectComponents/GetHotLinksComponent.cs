using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetHotLinksComponent : ArchicadAccessorComponent
    {
        public GetHotLinksComponent()
            : base(
                nameof(Hotlinks),
                nameof(Hotlinks),
                HotlinksResponse.Doc,
                GroupNames.Project)
        {
        }

        public static string CommandName => "GetHotlinks";

        public override Guid ComponentGuid =>
            new Guid("89ae1638-8c9f-481f-8a01-5cdf2ded8071");

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                nameof(Hotlink.Location) + "List",
                "",
                "",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "JsonHierarchy",
                "",
                "",
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!GetResponse(
                    CommandName,
                    null,
                    out HotlinksResponse response)) { return; }

            da.SetDataList(
                0,
                response.Hotlinks.GetLocations());
            da.SetData(
                1,
                JsonConvert.SerializeObject(
                    response,
                    Formatting.Indented));
        }
    }
}