using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;

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
        }

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

            SetValues(
                CommandName,
                new { issueId, author, text },
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AddCommentToAnIssue;

        public override Guid ComponentGuid =>
            new Guid("459bb412-6a24-41f8-b30c-8dd6c72994c2");
    }
}