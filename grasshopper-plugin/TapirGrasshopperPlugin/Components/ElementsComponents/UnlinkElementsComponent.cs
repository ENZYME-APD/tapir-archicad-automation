using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Types.Commands;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class UnlinkElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UnlinkElements";

        public UnlinkElementsComponent()
            : base(
                "UnlinkElements",
                "Remove the links created by LinkElements between pairs of elements. The direction matters: the link starting from the element of FromElementGuids " +
                "and pointing to the element of ToElementGuids at the same index is removed. Undoable.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            ElementLinkInputs.Add(this);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each link (empty when it was removed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var links = ElementLinkInputs.Read(
                this,
                da,
                0,
                1);
            if (links == null)
            {
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new ElementLinksParameters { Links = links },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UnlinkElements;

        public override Guid ComponentGuid =>
            new Guid("c4e8b2d6-5f1a-4c3e-9b7d-6a2e4f8c1d53");
    }
}
