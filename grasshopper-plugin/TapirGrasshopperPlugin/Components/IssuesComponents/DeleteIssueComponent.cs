using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class DeleteIssueComponent : ArchicadExecutorComponent
    {
        public class ParametersOfDelete
        {
            [JsonProperty ("issueId")]
            public IssueIdObj IssueId;

            [JsonProperty ("acceptAllElements")]
            public bool AcceptAllElements;
        }

        public DeleteIssueComponent ()
          : base (
                "Delete Issue",
                "DeleteIssue",
                "Delete Issue.",
                "Issues"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("IssueGuid", "IssueGuid", "Issue to delete.", GH_ParamAccess.item);
            pManager.AddBooleanParameter ("AcceptAllElements", "AcceptAllElements", "Accept all new/deleted Elements.", GH_ParamAccess.item, @default: true);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            IssueIdObj issueId = IssueIdObj.Create (DA, 0);
            if (issueId == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IssueGuid failed to collect data.");
                return;
            }

            bool acceptAllElements = true;
            if (!DA.GetData (1, ref acceptAllElements)) {
                return;
            }

            ParametersOfDelete parametersOfDelete = new ParametersOfDelete {
                IssueId = issueId,
                AcceptAllElements = acceptAllElements
            };
            JObject parameters = JObject.FromObject (parametersOfDelete);
            CommandResponse response = SendArchicadAddOnCommand ("DeleteIssue", parameters);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.DeleteIssue;

        public override Guid ComponentGuid => new Guid ("6cc670ea-6c05-408a-9041-3e7cd94a6f84");
    }
}
