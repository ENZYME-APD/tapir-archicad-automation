using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class DeleteIssueComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteIssue";

        public DeleteIssueComponent()
            : base(
                "DeleteIssue",
                "Delete Issue.",
                GroupNames.Issues)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "IssueGuid",
                "Issue to delete.");

            InBoolean(
                "AcceptAllElements",
                "Accept all new/deleted Elements.",
                true);
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
                    out bool acceptAllElements))
            {
                return;
            }

            SetValues(
                CommandName,
                new { issueId, acceptAllElements },
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteIssue;

        public override Guid ComponentGuid =>
            new Guid("6cc670ea-6c05-408a-9041-3e7cd94a6f84");
    }
}