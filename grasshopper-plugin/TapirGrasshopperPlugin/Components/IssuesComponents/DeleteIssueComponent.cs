using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Issues;

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

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is deleted: the component only checks that the issue exists, and reports " +
                "an error when it does not. Default is false.",
                false);

            SetOptionality(new[] { 1, 2 });
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

            bool acceptAllElements = da.GetOptional(
                1,
                true);

            var parameters = JObject.FromObject(new { issueId, acceptAllElements });

            // Only sent when it differs from the command's default. A dry
            // run answers with its result only, so it is checked here.
            if (da.GetOptional(2, false))
            {
                parameters["dryRun"] = true;
                if (TryGetCadResponse(
                        CommandName,
                        parameters,
                        ToAddOn,
                        out var response))
                {
                    this.CheckExecutionResult(response);
                }
                return;
            }

            SetCadValues(
                CommandName,
                parameters,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteIssue;

        public override Guid ComponentGuid =>
            new Guid("6cc670ea-6c05-408a-9041-3e7cd94a6f84");
    }
}