using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class AttachElementsToIssueComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "AttachElementsToIssue";


        public AttachElementsToIssueComponent()
            : base(
                "AttachElementsToAnIssue",
                "Attach Elements to an Issue.",
                GroupNames.Issues)
        {
        }

        protected override void AddInputs()
        {
            InGeneric("IssueGuid");

            InGenerics(
                "ElementGuids",
                "Elements to attach.");

            InGeneric(
                "Type",
                "Type of Elements.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new IssueElementTypeValueList().AddAsSource(
                this,
                2);
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

            if (!da.TryCreateFromList(
                    1,
                    out ElementsObject elements))
            {
                return;
            }

            if (!da.TryGet(
                    2,
                    out string type))
            {
                return;
            }

            SetValues(
                CommandName,
                new { issueId, type, elements = elements.Elements },
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AttachElementsToAnIssue;

        public override Guid ComponentGuid =>
            new Guid("15e08be9-913c-4766-93ce-edadc1a261c5");
    }
}