using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class GetAllIssuesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetIssues";

        public GetAllIssuesComponent()
            : base(
                "AllIssues",
                "Get all Issues.",
                GroupNames.Issues)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "IssueGuid",
                "Issue Guid.");

            OutGenerics(
                "Name",
                "Name.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedResponse(
                    CommandName,
                    out AllIssues response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Issues.Select(x => x.IssueId));

            da.SetDataList(
                1,
                response.Issues.Select(x => x.Name));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AllIssues;

        public override Guid ComponentGuid =>
            new Guid("5453804e-8983-4743-926d-a78b46fcecc1");
    }
}