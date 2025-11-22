using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class CreateIssueComponent : ArchicadExecutorComponent
    {
        public static string Doc => "Create an issue.";
        public override string CommandName => "CreateIssue";

        public CreateIssueComponent()
            : base(
                "Create Issue",
                "CreateIssue",
                Doc,
                GroupNames.Issues)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name");
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "IssueGuid",
                "Issue Guid.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.GetItem(
                    1,
                    out string name))
            {
                return;
            }

            if (!GetConvertedResponse(
                    CommandName,
                    new ParametersOfNewIssue { Name = name },
                    out IssueIdItemObj response))
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