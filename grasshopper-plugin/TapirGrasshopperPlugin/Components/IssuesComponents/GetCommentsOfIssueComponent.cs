using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class GetCommentsOfIssueComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCommentsFromIssue";

        public GetCommentsOfIssueComponent()
            : base(
                "GetCommentsOfAnIssue",
                "Get Comments of an Issue.",
                GroupNames.Issues)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "IssueGuid",
                "Issue Guid.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "Comments",
                "Comments of the given issue.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!IssueIdItemObj.TryCreate(
                    this,
                    da,
                    0,
                    out IssueIdItemObj issueId))
            {
                return;
            }

            if (!TryGetConvertedResponse(
                    CommandName,
                    issueId,
                    out IssueComments response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Comments);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCommentsOfAnIssue;

        public override Guid ComponentGuid =>
            new Guid("2c122dc0-ef6a-4e98-b35d-943ba30a99ee");
    }
}