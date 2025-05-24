using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class ProjectInfoField
    {
        [JsonProperty ("projectInfoId")]
        public string ProjectInfoId { get; set; }

        [JsonProperty ("projectInfoName")]
        public string ProjectInfoName { get; set; }

        [JsonProperty ("projectInfoValue")]
        public string ProjectInfoValue { get; set; }
    }

    public class ProjectInfoFields
    {
        [JsonProperty ("fields")]
        public List<ProjectInfoField> Fields { get; set; }
    }

    public class GetProjectInfoFieldsComponent : ArchicadAccessorComponent
        {
            public GetProjectInfoFieldsComponent ()
              : base (
                    "Project Info",
                    "ProjectInfo",
                    "Get project info.",
                    "Project"
                )
            {
            }

            protected override void RegisterInputParams (GH_InputParamManager pManager)
            {
            }

            protected override void RegisterOutputParams (GH_OutputParamManager pManager)
            {
                pManager.AddTextParameter ("Project Info Id", "ProjectInfoId", "Project Info Id.", GH_ParamAccess.list);
                pManager.AddTextParameter ("Project Info Name", "ProjectInfoName", "Project Info Name.", GH_ParamAccess.list);
                pManager.AddTextParameter ("Project Info Value", "ProjectInfoValue", "Project Info Value.", GH_ParamAccess.list);
            }

            protected override void Solve (IGH_DataAccess DA)
            {
                CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetProjectInfoFields", null);
                if (!response.Succeeded) {
                    AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                    return;
                }
                ProjectInfoFields projectInfoFields = response.Result.ToObject<ProjectInfoFields> ();
                List<string> projectInfoIds = new List<string> ();
                List<string> projectInfoNames = new List<string> ();
                List<string> projectInfoValues = new List<string> ();
                foreach (var projectInfoField in projectInfoFields.Fields) {
                    projectInfoIds.Add (projectInfoField.ProjectInfoId);
                    projectInfoNames.Add (projectInfoField.ProjectInfoName);
                    projectInfoValues.Add (projectInfoField.ProjectInfoValue);
                }
                DA.SetDataList (0, projectInfoIds);
                DA.SetDataList (1, projectInfoNames);
                DA.SetDataList (2, projectInfoValues);
            }

            // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ProjectInfo;

            public override Guid ComponentGuid => new Guid ("bd3881e2-8c87-4220-898e-91f3fd984fd9");
        }
}
