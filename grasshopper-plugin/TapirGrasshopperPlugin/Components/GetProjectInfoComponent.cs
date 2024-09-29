using Grasshopper;
using Grasshopper.Kernel;
using Newtonsoft.Json;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Text.Json;
using TapirGrasshopperPlugin.Components;
using TapirGrasshopperPlugin.Utilities;

namespace Tapir.Components
{
    public class ProjectInfo
    {
        [JsonProperty ("isUntitled")]
        public bool IsUntitled { get; set; }

        [JsonProperty ("isTeamwork")]
        public bool IsTeamwork { get; set; }

        [JsonProperty ("projectLocation")]
        public string ProjectLocation { get; set; }

        [JsonProperty ("projectPath")]
        public string ProjectPath { get; set; }

        [JsonProperty ("projectName")]
        public string ProjectName { get; set; }
    }

    public class GetProjectInfoComponent : ArchicadAccessorComponent
    {
        public GetProjectInfoComponent ()
          : base (
                "Get project information",
                "ProjectInfo",
                "Get information about the currently active project.",
                "General"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddBooleanParameter ("Is Untitled", "IsUntitled", "Is the project untitled.", GH_ParamAccess.item);
            pManager.AddBooleanParameter ("Is Teamwork", "IsTeamwork", "Is teamwork project.", GH_ParamAccess.item);
            pManager.AddTextParameter ("Project Location", "ProjectLocation", "Location of the project.", GH_ParamAccess.item);
            pManager.AddTextParameter ("Project Path", "ProjectPath", "Path of the project.", GH_ParamAccess.item);
            pManager.AddTextParameter ("Project Name", "ProjectName", "Name of the project.", GH_ParamAccess.item);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetProjectInfo", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            ProjectInfo projectInfo = response.Result.ToObject<ProjectInfo> ();
            DA.SetData (0, projectInfo.IsUntitled);
            DA.SetData (1, projectInfo.IsTeamwork);
            DA.SetData (2, projectInfo.ProjectLocation);
            DA.SetData (3, projectInfo.ProjectPath);
            DA.SetData (4, projectInfo.ProjectName);
        }

        protected override System.Drawing.Bitmap Icon => Properties.Resources.TapirLogo;

        public override Guid ComponentGuid => new Guid ("d46b6591-cae7-4809-8a3d-9b9b5ed77caf");
    }
}
