using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;
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
            InGeneric("IssueGuid");
            InGeneric("ElementType");
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
            if (!da.TryCreate(
                    0,
                    out IssueGuid issueId))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out string type))
            {
                return;
            }

            if (!TryGetConvertedValues(
                    CommandName,
                    new { issueId, type },
                    ToAddOn,
                    JHelp.Deserialize<ElementsObject>,
                    out ElementsObject response))
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