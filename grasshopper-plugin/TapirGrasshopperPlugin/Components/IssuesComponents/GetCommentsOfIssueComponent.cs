using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

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
            InGeneric("IssueGuid");
        }

        protected override void AddOutputs()
        {
            OutGenerics("Comments");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out IssueGuid input))
            {
                return;
            }

            if (!TryGetConvertedValues(
                    CommandName,
                    input,
                    ToAddOn,
                    JHelp.Deserialize<IssueComments>,
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