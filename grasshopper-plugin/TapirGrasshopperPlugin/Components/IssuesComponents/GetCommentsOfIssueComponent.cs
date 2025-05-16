using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class GetCommentsOfIssueComponent : ArchicadAccessorComponent
    {
        public GetCommentsOfIssueComponent ()
          : base (
                "Get Comments of an Issue",
                "GetComments",
                "Get Comments of an Issue.",
                "Issues"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("IssueGuid", "IssueGuid", "Issue Guid.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("Comments", "Comments", "Comments of the given issue.", GH_ParamAccess.list);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            IssueIdItemObj issueId = IssueIdItemObj.Create (DA, 0);
            if (issueId == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IssueGuid failed to collect data.");
                return;
            }

            JObject parameters = JObject.FromObject (issueId);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetCommentsFromIssue", parameters);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            IssueComments comments = response.Result.ToObject<IssueComments> ();
            DA.SetDataList (0, comments.Comments);
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.GetComments;

        public override Guid ComponentGuid => new Guid ("2c122dc0-ef6a-4e98-b35d-943ba30a99ee");
    }
}
