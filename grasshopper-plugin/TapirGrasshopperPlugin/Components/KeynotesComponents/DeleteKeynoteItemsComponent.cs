using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Keynotes;

namespace TapirGrasshopperPlugin.Components.KeynotesComponents
{
    public class DeleteKeynoteItemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteKeynoteItems";

        public DeleteKeynoteItemsComponent()
            : base(
                "DeleteKeynoteItems",
                "Delete the given keynote items. " +
                "Available from Archicad 28.",
                GroupNames.Keynotes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ItemGuids",
                "Identifiers of the keynote items to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each keynote item (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out KeynoteItemIdsObject input))
            {
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteKeynoteItems;

        public override Guid ComponentGuid =>
            new Guid("323f05f1-3906-4287-9487-76d7ff21ccf1");
    }
}
