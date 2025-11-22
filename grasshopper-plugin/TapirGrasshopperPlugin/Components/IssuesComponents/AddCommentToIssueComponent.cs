using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class AddCommentToIssueComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "AddCommentToIssue";

        public AddCommentToIssueComponent()
            : base(
                "AddCommentToAnIssue",
                "Add Comment to an Issue.",
                GroupNames.Issues)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "IssueGuid",
                "Issue Guid.");

            InGeneric(
                "Author",
                "Author");

            InGeneric(
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