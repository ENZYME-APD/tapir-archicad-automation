using System;

namespace TapirGrasshopperPlugin.Components.RevisionComponents
{
    public class GetRevisionIssuesComponent : NoInputJsonAccessorComponent
    {
        public override string CommandName => "GetRevisionIssues";

        public GetRevisionIssuesComponent()
            : base(
                "GetRevisionIssues",
                "Retrieve all revision issues.",
                GroupNames.Revision,
                "RevisionIssues")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetRevisionIssues;

        public override Guid ComponentGuid =>
            new Guid("93ff2a2d-6342-48ab-9986-9a77cf1b1e09");
    }
}
