using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class DeleteNavigatorItem : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteNavigatorItems";

        public DeleteNavigatorItem()
            : base(
                "DeleteNavigatorItems",
                "Deletes navigator items from the navigator tree: views and folders of the View Map, layouts and " +
                "master layouts of the Layout Book, and independent worksheets and details of the Project Map. " +
                "Layout subsets and marker-linked worksheets and details cannot be deleted. An item that Archicad " +
                "refuses to delete (in a Teamwork project: it is not reserved or the user has no right to delete " +
                "it) is reported as a failed execution result.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemIds",
                "Identifier of navigator items to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each navigator item (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out NavigatorItemsObject input))
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
            Properties.Resources.DeleteNavigatorItems;

        public override Guid ComponentGuid =>
            new Guid("b4ff32b4-91ac-405d-96ed-1938aec11eb3");
    }
}