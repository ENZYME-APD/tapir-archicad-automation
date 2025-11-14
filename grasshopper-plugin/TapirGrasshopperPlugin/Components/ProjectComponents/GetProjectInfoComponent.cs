using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetProjectInfoComponent : ArchicadAccessorComponent
    {
        public static string CommandName => "GetProjectInfo";

        public GetProjectInfoComponent () :
            base ("Project Details", "ProjectDetails", ProjectInfo.Doc, GroupNames.Project)
        { }

        protected override void RegisterInputParams (GH_InputParamManager pManager) { }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddBooleanParameter ("Is Untitled", "IsUntitled", "Is the project untitled.", GH_ParamAccess.item);
            pManager.AddBooleanParameter ("Is Teamwork", "IsTeamwork", "Is teamwork project.", GH_ParamAccess.item);
            pManager.AddTextParameter ("Project Location", "ProjectLocation", "Location of the project.", GH_ParamAccess.item);
            pManager.AddTextParameter ("Project Path", "ProjectPath", "Path of the project.", GH_ParamAccess.item);
            pManager.AddTextParameter ("Project Name", "ProjectName", "Name of the project.", GH_ParamAccess.item);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            if (!GetResponse (CommandName, null, out ProjectInfo projectInfo)) return;

            DA.SetData (0, projectInfo.IsUntitled);
            DA.SetData (1, projectInfo.IsTeamwork);
            DA.SetData (2, projectInfo.ProjectLocation);
            DA.SetData (3, projectInfo.ProjectPath);
            DA.SetData (4, projectInfo.ProjectName);
        }

        protected override System.Drawing.Bitmap Icon => Properties.Resources.ProjectDetails;

        public override Guid ComponentGuid => new Guid ("d46b6591-cae7-4809-8a3d-9b9b5ed77caf");
    }
}
