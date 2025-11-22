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
            var navigatorItemIds = NavigatorItemIdsObj.Create(
                da,
                0);
            if (navigatorItemIds == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input NavigatorItemIds failed to collect data.");
                return;
            }

            GetResponse(
                CommandName,
                navigatorItemIds);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteNavigatorItems;

        public override Guid ComponentGuid =>
            new Guid("b4ff32b4-91ac-405d-96ed-1938aec11eb3");
    }
}