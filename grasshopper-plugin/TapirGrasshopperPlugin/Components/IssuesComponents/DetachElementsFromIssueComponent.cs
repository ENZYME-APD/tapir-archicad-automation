using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
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
            if (!IssueIdObj.TryCreate(
                    da,
                    0,
                    out IssueIdObj id))
            {
                return;
            }

            if (!ElementsObj.TryCreate(
                    da,
                    1,
                    out ElementsObj elements))
            {
                return;
            }

            SetArchiCadValues(
                CommandName,
                new ParametersOfDetachElements
                {
                    IssueId = id, Elements = elements.Elements
                });
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DetachElementsFromAnIssue;

        public override Guid ComponentGuid =>
            new Guid("83189f2c-5a8a-4315-a506-2a30a2737ae6");
    }
}