using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class DeleteNavigatorItem : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteNavigatorItems";

        public DeleteNavigatorItem()
            : base(
                "DeleteNavigatorItems",
                "Deletes items from navigator tree.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemIds",
                "Identifier of navigator items to delete.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!NavigatorItemIdsObj.TryCreate(
                    da,
                    0,
                    out NavigatorItemIdsObj navigatorItemIds))
            {
                return;
            }

            SetArchiCadValues(
                CommandName,
                navigatorItemIds);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteNavigatorItems;

        public override Guid ComponentGuid =>
            new Guid("b4ff32b4-91ac-405d-96ed-1938aec11eb3");
    }
}