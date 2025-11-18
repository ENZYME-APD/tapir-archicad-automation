using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class CreateIssueComponent : ArchicadExecutorComponent
    {
        public static string Doc => "Create an issue.";
        public override string CommandName => "CreateIssue";

        public class ParametersOfNewIssue
        {
            [JsonProperty("name")]
            public string Name;
        }

        public CreateIssueComponent()
            : base(
                "Create Issue",
                "CreateIssue",
                Doc,
                GroupNames.Issues)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Name",
                "Name",
                "Name",
                GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "IssueGuid",
                "IssueGuid",
                "Issue Guid.",
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var name = "";
            if (!da.GetData(
                    0,
                    ref name))
            {
                return;
            }

            var parametersOfNewIssue = new ParametersOfNewIssue { Name = name };

            if (!GetConvertedResponse(
                    CommandName,
                    parametersOfNewIssue,
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