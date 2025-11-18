using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class AddCommentToIssueComponent : ArchicadExecutorComponent
    {
        public static string Doc => "Add Comment to an Issue.";
        public override string CommandName => "AddCommentToIssue";

        public class ParametersOfNewComment
        {
            [JsonProperty("issueId")]
            public IssueIdObj IssueId;

            [JsonProperty("author")]
            public string Author;

            [JsonProperty("text")]
            public string Text;
        }

        public AddCommentToIssueComponent()
            : base(
                "Add Comment to an Issue",
                "AddComment",
                Doc,
                GroupNames.Issues)
        {
        }

        protected override void AddInputs()
        {
            AddGeneric(
                "IssueGuid",
                "Issue Guid.");

            AddGeneric(
                "Author",
                "Author");

            AddGeneric(
                "Text",
                "Text");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var issueId = IssueIdObj.Create(
                da,
                0);
            if (issueId == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input IssueGuid failed to collect data.");
                return;
            }

            var author = "";
            if (!da.GetData(
                    1,
                    ref author))
            {
                return;
            }

            var text = "";
            if (!da.GetData(
                    2,
                    ref text))
            {
                return;
            }

            var parametersOfNewComment = new ParametersOfNewComment
            {
                IssueId = issueId, Author = author, Text = text
            };

            GetResponse(
                CommandName,
                parametersOfNewComment);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AddCommentToAnIssue;

        public override Guid ComponentGuid =>
            new Guid("459bb412-6a24-41f8-b30c-8dd6c72994c2");
    }
}