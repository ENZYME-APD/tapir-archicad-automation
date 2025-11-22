using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
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
                "Name");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!GetConvertedResponse(
                    CommandName,
                    out AllIssues response))
            {
                return;
            }

            var issueIds = new List<IssueIdObj>();
            var issueNames = new List<string>();

            foreach (var detail in response.Issues)
            {
                issueIds.Add(detail.IssueId);
                issueNames.Add(detail.Name);
            }

            da.SetDataList(
                0,
                issueIds);
            da.SetDataList(
                1,
                issueNames);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AllIssues;

        public override Guid ComponentGuid =>
            new Guid("5453804e-8983-4743-926d-a78b46fcecc1");
    }
}