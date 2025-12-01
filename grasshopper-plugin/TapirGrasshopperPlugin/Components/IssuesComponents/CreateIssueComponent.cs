using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class CreateIssueComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateIssue";

        public CreateIssueComponent()
            : base(
                "CreateIssue",
                "Create an issue.",
                GroupNames.Issues)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name.");
        }

        protected override void AddOutputs()
        {
            OutGeneric("IssueGuid");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    1,
                    out string name))
            {
                return;
            }

            if (!TryGetConvertedValues(
                    CommandName,
                    new { name },
                    ToAddOn,
                    JHelp.Deserialize<IssueGuidItemObject>,
                    out IssueGuidItemObject response))
            {
                return;
            }

            da.SetData(
                0,
                response.IssueId);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateIssue;

        public override Guid ComponentGuid =>
            new Guid("fdd43474-b0de-4354-8856-c1dc0b07195f");
    }
}