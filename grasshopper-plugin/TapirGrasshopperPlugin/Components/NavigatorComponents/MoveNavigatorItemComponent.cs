using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
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
                "If it's not given then moves it at the first place under the new parent.");

            Params.Input[1].Optional = true;
            Params.Input[2].Optional = true;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!NavigatorIdItemObj.TryCreate(
                    da,
                    0,
                    out NavigatorIdItemObj navigatorItemId))
            {
                return;
            }

            if (!NavigatorIdItemObj.TryCreate(
                    da,
                    1,
                    out NavigatorIdItemObj parentNavigatorItemId))
            {
                return;
            }

            var previousNavigatorItemId = NavigatorIdItemObj.Create(
                da,
                2);

            var input = new MoveNavigatorItemInput()
            {
                NavigatorItemIdToMove = navigatorItemId.Id,
                ParentNavigatorItemId = parentNavigatorItemId.Id,
                PreviousNavigatorItemId = previousNavigatorItemId == null
                    ? null
                    : previousNavigatorItemId.Id
            };

            SetArchiCadValues(
                CommandName,
                input);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("fef582f6-07bd-4ffd-92cd-e41cb0447e2d");
    }
}