using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
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
                "Author.");

            InGeneric(
                "Text",
                "Text.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!IssueIdObj.TryCreate(
                    da,
                    0,
                    out IssueIdObj issueId))
            {
                return;
            }

            if (!da.TryGetItem(
                    1,
                    out string author))
            {
                return;
            }

            if (!da.TryGetItem(
                    2,
                    out string text))
            {
                return;
            }

            var parameters = new ParametersOfNewComment
            {
                IssueId = issueId, Author = author, Text = text
            };

            SetArchiCadValues(
                CommandName,
                parameters);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AddCommentToAnIssue;

        public override Guid ComponentGuid =>
            new Guid("459bb412-6a24-41f8-b30c-8dd6c72994c2");
    }
}