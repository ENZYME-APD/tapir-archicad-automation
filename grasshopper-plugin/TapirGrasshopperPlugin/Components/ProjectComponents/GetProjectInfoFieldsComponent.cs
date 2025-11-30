using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetProjectInfoFieldsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetProjectInfoFields";

        public GetProjectInfoFieldsComponent()
            : base(
                "ProjectInfo",
                "Get project info.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts("Ids");
            OutTexts("Names");
            OutTexts("Values");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedValues(
                    CommandName,
                    null,
                    SendToAddOn,
                    JHelp.Deserialize<ProjectInfoFields>,
                    out ProjectInfoFields response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Fields.Select(x => x.ProjectInfoId));

            da.SetDataList(
                1,
                response.Fields.Select(x => x.ProjectInfoName));

            da.SetDataList(
                2,
                response.Fields.Select(x => x.ProjectInfoValue));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ProjectInfo;

        public override Guid ComponentGuid =>
            new Guid("bd3881e2-8c87-4220-898e-91f3fd984fd9");
    }
}