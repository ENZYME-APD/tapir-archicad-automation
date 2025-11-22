using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
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

            var newName = "";
            if (!da.GetData(
                    1,
                    ref newName))
            {
                return;
            }

            var newId = "";
            if (!da.GetData(
                    2,
                    ref newId))
            {
                return;
            }

            var input = new RenameNavigatorItemInput()
            {
                NavigatorItemId = navigatorItemId.Id,
                NewName = String.IsNullOrEmpty(newName) ? null : newName,
                NewId = String.IsNullOrEmpty(newId) ? null : newId
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