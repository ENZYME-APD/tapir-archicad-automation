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
                "Deletes items from the NavigatorTree.",
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
                ToArchicad,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteNavigatorItems;

        public override Guid ComponentGuid =>
            new Guid("b4ff32b4-91ac-405d-96ed-1938aec11eb3");
    }
}