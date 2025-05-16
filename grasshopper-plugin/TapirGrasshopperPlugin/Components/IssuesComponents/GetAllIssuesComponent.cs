using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class GetAllIssuesComponent : ArchicadAccessorComponent
    {
        public GetAllIssuesComponent ()
          : base (
                "All Issues",
                "AllIssues",
                "Get all issues.",
                "Issues"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("IssueGuid", "IssueGuid", "Issue Guid.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("Name", "Name", "Name", GH_ParamAccess.list);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetIssues", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            AllIssues issues = response.Result.ToObject<AllIssues> ();
            List<IssueIdObj> issueIds = new List<IssueIdObj> ();
            List<string> issueNames = new List<string> ();
            foreach (IssueDetailsObj detail in issues.Issues) {
                issueIds.Add (detail.IssueId);
                issueNames.Add (detail.Name);
            }

            DA.SetDataList (0, issueIds);
            DA.SetDataList (1, issueNames);
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.AllIssues;

        public override Guid ComponentGuid => new Guid ("5453804e-8983-4743-926d-a78b46fcecc1");
    }
}
