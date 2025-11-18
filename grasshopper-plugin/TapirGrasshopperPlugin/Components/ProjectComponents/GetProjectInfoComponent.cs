using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetProjectInfoComponent : ArchicadAccessorComponent
    {
        public static string Doc =>
            "Get details of the currently active project.";

        public override string CommandName => "GetProjectInfo";

        public GetProjectInfoComponent()
            : base(
                "Project Details",
                "ProjectDetails",
                Doc,
                GroupNames.Project)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddBooleanParameter(
                "Is Untitled",
                "IsUntitled",
                "Is the project untitled.",
                GH_ParamAccess.item);
            pManager.AddBooleanParameter(
                "Is Teamwork",
                "IsTeamwork",
                "Is teamwork project.",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "Project Location",
                "ProjectLocation",
                "Location of the project.",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "Project Path",
                "ProjectPath",
                "Path of the project.",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "Project Name",
                "ProjectName",
                "Name of the project.",
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!GetConvertedResponse(
                    CommandName,
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