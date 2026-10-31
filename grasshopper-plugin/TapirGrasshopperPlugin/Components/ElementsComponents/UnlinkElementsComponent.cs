using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class UnlinkElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UnlinkElements";

        public UnlinkElementsComponent()
            : base(
                "UnlinkElements",
                "Removes the links created by LinkElements between the given pairs of elements. The direction " +
                "matters: the link starting from fromElementId and pointing to toElementId is removed. " +
                "Undoable.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            ElementLinksHelp.AddLinkInputs(this);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each link (empty when it was removed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var links = ElementLinksHelp.Links(
                this,
                da);
            if (links == null)
            {
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["links"] = links },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UnlinkElements;

        public override Guid ComponentGuid =>
            new Guid("99a1c10a-6279-44ff-a20d-38cb38399298");
    }
}
