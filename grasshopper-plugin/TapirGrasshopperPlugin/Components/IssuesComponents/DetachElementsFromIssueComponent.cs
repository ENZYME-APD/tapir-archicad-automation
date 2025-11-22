using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class DetachElementsFromIssueComponent : ArchicadExecutorComponent
    {
        public static string Doc => "Detach Elements from an Issue.";
        public override string CommandName => "DetachElementsFromIssue";

        public DetachElementsFromIssueComponent()
            : base(
                "Detach Elements from an Issue",
                "DetachElements",
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

            var parameters = new ParametersOfDetachElements
            {
                IssueId = issueId, Elements = elements.Elements
            };

            GetResponse(
                CommandName,
                parameters);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DetachElementsFromAnIssue;

        public override Guid ComponentGuid =>
            new Guid("83189f2c-5a8a-4315-a506-2a30a2737ae6");
    }
}