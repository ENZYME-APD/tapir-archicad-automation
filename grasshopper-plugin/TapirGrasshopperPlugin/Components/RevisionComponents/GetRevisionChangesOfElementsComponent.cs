using System;

namespace TapirGrasshopperPlugin.Components.RevisionComponents
{
    public class GetRevisionChangesOfElementsComponent : ElementsJsonAccessorComponent
    {
        public override string CommandName => "GetRevisionChangesOfElements";

        public GetRevisionChangesOfElementsComponent()
            : base(
                "GetRevisionChangesOfElements",
                "Retrieve the revision changes belonging to the given elements.",
                GroupNames.Revision,
                "RevisionChanges")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetRevisionChangesOfElements;

        public override Guid ComponentGuid =>
            new Guid("14da2b8e-a89e-4b0c-9460-2d8199517c90");
    }
}
