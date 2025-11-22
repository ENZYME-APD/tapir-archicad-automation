using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class DeleteIssueComponent : ArchicadExecutorComponent
    {
        public static string Doc => "Delete Issue.";
        public override string CommandName => "DeleteIssue";

        public DeleteIssueComponent()
            : base(
                "Delete Issue",
                "DeleteIssue",
                Doc,
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

            var acceptAllElements = true;
            if (!da.GetData(
                    1,
                    ref acceptAllElements))
            {
                return;
            }

            var parameters = new ParametersOfDelete
            {
                IssueId = issueId, AcceptAllElements = acceptAllElements
            };

            GetResponse(
                CommandName,
                parameters);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteIssue;

        public override Guid ComponentGuid =>
            new Guid("6cc670ea-6c05-408a-9041-3e7cd94a6f84");
    }
}