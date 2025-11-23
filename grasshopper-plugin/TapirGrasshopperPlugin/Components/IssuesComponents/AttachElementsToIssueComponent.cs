using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
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
            InGeneric(
                "IssueGuid",
                "Issue Guid.");

            InGenerics(
                "ElementGuids",
                "Elements to attach.");

            InGeneric(
                "Type",
                "Type of elements.");
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
                    out IssueIdObj issueId))
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

            if (!da.TryGetItem(
                    2,
                    out string type))
            {
                return;
            }

            var parameters = new ParametersOfAttachElements
            {
                IssueId = issueId, Elements = elements.Elements, Type = type
            };

            SetArchiCadValues(
                CommandName,
                parameters);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AttachElementsToAnIssue;

        public override Guid ComponentGuid =>
            new Guid("15e08be9-913c-4766-93ce-edadc1a261c5");
    }
}