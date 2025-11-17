using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetProjectInfoFieldsComponent : ArchicadAccessorComponent
    {
        public static string CommandName => "GetProjectInfoFields";

        public GetProjectInfoFieldsComponent ()
            : base ("Project Info", "ProjectInfo", "Get project info.",
                GroupNames.Project)
        { }

        protected override void RegisterInputParams (GH_InputParamManager pManager) { }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        { 
            pManager.AddTextParameter ("Project Info Id", "ProjectInfoId",
                "Project Info Id.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Project Info Name", "ProjectInfoName",
                "Project Info Name.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Project Info Value", "ProjectInfoValue",
                "Project Info Value.", GH_ParamAccess.list);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            if (!GetResponse (CommandName, null,
                    out ProjectInfoFields projectInfoFields))
            {
                return;
            }

            var ids = new List<string> ();
            var names = new List<string> ();
            var values = new List<string> ();

            foreach (var field in projectInfoFields.Fields)
            {
                ids.Add (field.ProjectInfoId);
                names.Add (field.ProjectInfoName);
                values.Add (field.ProjectInfoValue);
            }
            
            DA.SetDataList (0, ids);
            DA.SetDataList (1, names);
            DA.SetDataList (2, values);
        }

        protected override System.Drawing.Bitmap Icon => Properties.Resources.ProjectInfo;

        public override Guid ComponentGuid => new Guid ("bd3881e2-8c87-4220-898e-91f3fd984fd9");
    }
}
