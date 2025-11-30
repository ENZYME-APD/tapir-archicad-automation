using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class RenameNavigatorItem : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameNavigatorItem";

        public RenameNavigatorItem()
            : base(
                "RenameNavigatorItem",
                "Renames a navigator item.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "NavigatorItemIds",
                "Identifier of navigator items to rename.");

            InText(
                "NewName",
                "New name for the navigator item.");

            InText(
                "NewId",
                "New id for the navigator item.");
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

            if (!da.TryGetItem(
                    1,
                    out string newName))
            {
                return;
            }

            if (!da.TryGetItem(
                    2,
                    out string newId))
            {
                return;
            }

            var input = new RenameNavigatorItemInput
            {
                NavigatorItemId = navigatorItemId.Id,
                NewName = string.IsNullOrEmpty(newName) ? null : newName,
                NewId = string.IsNullOrEmpty(newId) ? null : newId
            };

            SetValues(
                CommandName,
                input,
                SendToArchicad);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("6be1cc4e-6645-400f-b632-1fccdae0db4d");
    }
}