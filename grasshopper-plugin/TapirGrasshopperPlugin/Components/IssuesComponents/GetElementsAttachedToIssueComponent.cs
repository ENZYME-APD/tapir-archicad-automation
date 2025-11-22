using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class GetElementsAttachedToIssueComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Get Elements Attached to an Issue.";
        public override string CommandName => "GetElementsAttachedToIssue";

        public class ParametersOfGetAttachedElements
        {
            [JsonProperty("issueId")]
            public IssueIdObj IssueId;

            [JsonProperty("type")]
            public string Type;
        }

        public GetElementsAttachedToIssueComponent()
            : base(
                "Get Elements Attached to an Issue",
                "GetAttachedElements",
                Doc,
                GroupNames.Issues)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "IssueGuid",
                "Issue Guid.");

            InGeneric(
                "Type",
                "Type of elements.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Attached elements of the given issue.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new IssueElementTypeValueList().AddAsSource(
                this,
                1);
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

            var type = "";
            if (!da.GetData(
                    1,
                    ref type))
            {
                return;
            }

            var parameters =
                new ParametersOfGetAttachedElements
                {
                    IssueId = issueId, Type = type
                };

            if (!GetConvertedResponse(
                    CommandName,
                    parameters,
                    out ElementsObj response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Elements);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementsAttachedToAnIssue;

        public override Guid ComponentGuid =>
            new Guid("097b773b-3cd4-43a5-a41d-1fb57bc3e2e2");
    }
}