using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Issues;

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

            InGeneric(
                "ParentIssueGuid",
                "The new issue is created under this issue. Optional.");

            InText(
                "TagText",
                "Tag text of the new issue. Optional.");

            SetOptionality(
                new[]
                {
                    1,
                    2
                });
        }

        protected override void AddOutputs()
        {
            OutGeneric("IssueGuid");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string name))
            {
                return;
            }

            // Unset means a top-level issue; a given value that is not an
            // issue is an error, not silently a top-level issue.
            IssueGuid parentIssueId = null;
            if (da.TryGet(
                    1,
                    out GH_ObjectWrapper parentWrapper) &&
                !parentWrapper.TryBuildObject(out parentIssueId))
            {
                this.AddError("ParentIssueGuid is not a valid issue identifier.");
                return;
            }

            var tagText = da.GetOptional<string>(
                2,
                null);

            if (string.IsNullOrEmpty(tagText))
            {
                tagText = null;
            }

            // The unset optional fields are null, which is not sent.
            if (!TryGetConvertedCadValues(
                    CommandName,
                    new { name, parentIssueId, tagText },
                    ToAddOn,
                    JHelp.Deserialize<IssueGuidWrapper>,
                    out IssueGuidWrapper response))
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