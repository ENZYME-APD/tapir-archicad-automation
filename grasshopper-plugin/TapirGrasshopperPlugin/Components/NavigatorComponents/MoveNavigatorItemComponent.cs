using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class MoveNavigatorItem : ArchicadExecutorComponent
    {
        public override string CommandName => "MoveNavigatorItem";

        public MoveNavigatorItem()
            : base(
                "MoveNavigatorItem",
                "Moves the given navigator item under the parentNavigatorItemId in the navigator tree. " +
                "If previousNavigatorItemId is not given then inserts it at the first place under the new parent. " +
                "If it is given then inserts it after this navigator item.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "NavigatorItemIds",
                "Identifier of navigator items to move.");

            InGeneric(
                "ParentNavigatorItemId",
                "Moves the given navigator item under the parentNavigatorItemId in the navigator tree.");


            InGeneric(
                "PreviousNavigatorItemId",
                "Moves the given navigator item after this navigator item. " +
                "By default it moves it at the first place under the new parent.");

            Params.Input[2].Optional = true;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out NavigatorGuid navigatorItemIdToMove))
            {
                return;
            }

            if (!da.TryCreate(
                    1,
                    out NavigatorGuid parentNavigatorItemId))
            {
                return;
            }

            if (!da.TryCreate(
                    2,
                    out NavigatorGuid previousNavigatorItemId))
            {
                previousNavigatorItemId = null;
            }

            SetValues(
                CommandName,
                new
                {
                    navigatorItemIdToMove,
                    parentNavigatorItemId,
                    previousNavigatorItemId
                },
                ToArchicad);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("fef582f6-07bd-4ffd-92cd-e41cb0447e2d");
    }
}