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
                "New Name",
                "New name for the navigator item.",
                "");

            InText(
                "New Id",
                "New id for the navigator item.",
                "");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var navigatorItemId = NavigatorIdItemObj.Create(
                da,
                0);
            if (navigatorItemId == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input NavigatorItemId failed to collect data.");
                return;
            }

            if (!da.GetItem(
                    1,
                    out string newName))
            {
                return;
            }

            if (!da.GetItem(
                    2,
                    out string newId))
            {
                return;
            }

            var input = new RenameNavigatorItemInput()
            {
                NavigatorItemId = navigatorItemId.Id,
                NewName = string.IsNullOrEmpty(newName) ? null : newName,
                NewId = string.IsNullOrEmpty(newId) ? null : newId
            };

            GetResponse(
                CommandName,
                input);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("6be1cc4e-6645-400f-b632-1fccdae0db4d");
    }
}