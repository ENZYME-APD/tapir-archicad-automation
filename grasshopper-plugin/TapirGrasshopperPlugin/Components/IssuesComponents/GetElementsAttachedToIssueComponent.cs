using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class GetElementsAttachedToIssueComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementsAttachedToIssue";

        public GetElementsAttachedToIssueComponent()
            : base(
                "GetElementsAttachedToAnIssue",
                "Get Elements Attached to an Issue.",
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

            if (!da.GetItem(
                    1,
                    out string type))
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