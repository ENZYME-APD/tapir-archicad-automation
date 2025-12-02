using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class DetachElementsFromIssueComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DetachElementsFromIssue";

        public DetachElementsFromIssueComponent()
            : base(
                "DetachElementsFromAnIssue",
                "Detach Elements from an Issue.",
                GroupNames.Issues)
        {
        }

        protected override void AddInputs()
        {
            InGeneric("IssueGuid");

            InGenerics(
                "ElementGuids",
                "Elements to detach.");
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
                    out ElementsObj elements))
            {
                return;
            }

            SetValues(
                CommandName,
                new { issueId, elements = elements.Elements },
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DetachElementsFromAnIssue;

        public override Guid ComponentGuid =>
            new Guid("83189f2c-5a8a-4315-a506-2a30a2737ae6");
    }
}