using System;

namespace TapirGrasshopperPlugin.Components.RevisionComponents
{
    public class GetDocumentRevisionsComponent : NoInputJsonAccessorComponent
    {
        public override string CommandName => "GetDocumentRevisions";

        public GetDocumentRevisionsComponent()
            : base(
                "GetDocumentRevisions",
                "Retrieve all document revisions.",
                GroupNames.Revision,
                "DocumentRevisions")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDocumentRevisions;

        public override Guid ComponentGuid =>
            new Guid("866db71d-ff77-4077-9bde-ad08ae5c6822");
    }
}
