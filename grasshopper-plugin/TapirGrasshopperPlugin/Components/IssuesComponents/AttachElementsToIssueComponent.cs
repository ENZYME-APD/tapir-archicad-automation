using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class AttachElementsToIssueComponent : ArchicadExecutorComponent
    {
        public static string Doc => "Attach Elements to an Issue.";
        public override string CommandName => "AttachElementsToIssue";


        public AttachElementsToIssueComponent()
            : base(
                "Attach Elements to an Issue",
                "AttachElements",
                Doc,
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

            var elements = ElementsObj.Create(
                da,
                1);
            if (elements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            var type = "";
            if (!da.GetData(
                    2,
                    ref type))
            {
                return;
            }

            var parametersOfAttachElements = new ParametersOfAttachElements
            {
                IssueId = issueId, Elements = elements.Elements, Type = type
            };

            GetResponse(
                CommandName,
                parametersOfAttachElements);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AttachElementsToAnIssue;

        public override Guid ComponentGuid =>
            new Guid("15e08be9-913c-4766-93ce-edadc1a261c5");
    }
}