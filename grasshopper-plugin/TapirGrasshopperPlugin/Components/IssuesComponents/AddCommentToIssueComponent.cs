using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Issues;

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
            InGeneric("IssueGuid");
            InGeneric("Author");
            InGeneric("Text");

            InText(
                "Status",
                "Status of the comment: " + string.Join(", ", CommentStatuses) + ". Optional.");

            SetOptionality(new[] { 3 });
        }

        private static readonly string[] CommentStatuses =
        {
            "Error",
            "Warning",
            "Info",
            "Unknown"
        };

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out IssueGuid issueId))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out string author))
            {
                return;
            }

            if (!da.TryGet(
                    2,
                    out string text))
            {
                return;
            }

            var status = da.GetOptional<string>(
                3,
                null);

            if (string.IsNullOrEmpty(status))
            {
                status = null;
            }
            else if (!CommentStatuses.Contains(status))
            {
                this.AddError(
                    "Status must be one of: " + string.Join(", ", CommentStatuses) + ".");
                return;
            }

            // An unset status is null, which is not sent.
            SetCadValues(
                CommandName,
                new { issueId, author, text, status },
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AddCommentToAnIssue;

        public override Guid ComponentGuid =>
            new Guid("459bb412-6a24-41f8-b30c-8dd6c72994c2");
    }
}