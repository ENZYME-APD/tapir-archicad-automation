using System;

namespace TapirGrasshopperPlugin.Components.RevisionComponents
{
    public class GetRevisionChangesComponent : NoInputJsonAccessorComponent
    {
        public override string CommandName => "GetRevisionChanges";

        public GetRevisionChangesComponent()
            : base(
                "GetRevisionChanges",
                "Retrieve all revision changes.",
                GroupNames.Revision,
                "RevisionChanges")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetRevisionChanges;

        public override Guid ComponentGuid =>
            new Guid("335b9748-401a-4de8-8bb1-b6b83a7e5d17");
    }
}
