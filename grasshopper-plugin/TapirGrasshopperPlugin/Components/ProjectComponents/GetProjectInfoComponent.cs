using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetProjectInfoComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetProjectInfo";

        public GetProjectInfoComponent()
            : base(
                "ProjectDetails",
                "Get details of the currently active project.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean("IsUntitled");
            OutBoolean("IsTeamwork");
            OutText("ProjectLocation");
            OutText("ProjectPath");
            OutText("ProjectName");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<ProjectInfo>,
                    out ProjectInfo response))
            {
                return;
            }

            da.SetData(
                0,
                response.IsUntitled);
            da.SetData(
                1,
                response.IsTeamwork);
            da.SetData(
                2,
                response.ProjectLocation);
            da.SetData(
                3,
                response.ProjectPath);
            da.SetData(
                4,
                response.ProjectName);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ProjectDetails;

        public override Guid ComponentGuid =>
            new Guid("d46b6591-cae7-4809-8a3d-9b9b5ed77caf");
    }
}