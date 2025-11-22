using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetHotLinksComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetHotlinks";

        public GetHotLinksComponent()
            : base(
                nameof(Hotlinks),
                "Gets the file system locations (path) of the link modules. " +
                "The hotlinks can have tree hierarchy in the project.",
                GroupNames.Project)
        {
        }

        public override Guid ComponentGuid =>
            new Guid("89ae1638-8c9f-481f-8a01-5cdf2ded8071");


        protected override void AddOutputs()
        {
            OutTexts(
                nameof(Hotlink.Location) + "s",
                "");

            OutText(
                "JsonHierarchy",
                "");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!GetConvertedResponse(
                    CommandName,
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