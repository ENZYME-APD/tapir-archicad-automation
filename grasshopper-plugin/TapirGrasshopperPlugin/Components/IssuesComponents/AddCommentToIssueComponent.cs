using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class AddCommentToIssueComponent : ArchicadExecutorComponent
    {
        public class ParametersOfNewComment
        {
            [JsonProperty ("issueId")]
            public IssueIdObj IssueId;

            [JsonProperty ("author")]
            public string Author;

            [JsonProperty ("text")]
            public string Text;
        }

        public AddCommentToIssueComponent ()
          : base (
                "Add Comment to an Issue",
                "AddComment",
                "Add Comment to an Issue.",
                "Issues"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("IssueGuid", "IssueGuid", "Issue Guid.", GH_ParamAccess.item);
            pManager.AddGenericParameter ("Author", "Author", "Author.", GH_ParamAccess.item);
            pManager.AddGenericParameter ("Text", "Text", "Text.", GH_ParamAccess.item);
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

            string author = "";
            if (!DA.GetData (1, ref author)) {
                return;
            }

            string text = "";
            if (!DA.GetData (2, ref text)) {
                return;
            }

            ParametersOfNewComment parametersOfNewComment = new ParametersOfNewComment {
                IssueId = issueId,
                Author = author,
                Text = text
            };
            JObject parameters = JObject.FromObject (parametersOfNewComment);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "AddCommentToIssue", parameters);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.AddCommentToAnIssue;

        public override Guid ComponentGuid => new Guid ("459bb412-6a24-41f8-b30c-8dd6c72994c2");
    }
}
