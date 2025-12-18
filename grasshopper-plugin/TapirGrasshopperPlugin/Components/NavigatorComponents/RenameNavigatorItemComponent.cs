using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

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
                "NavigatorItemId",
                "Identifier of navigator items to rename.");

            InTextWithDefault(
                "NewName",
                "New name for the navigator item.",
                defaultValue: "");

            InTextWithDefault(
                "NewId",
                "New id for the navigator item.",
                defaultValue: "");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out NavigatorGuid navigatorItemId))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out string newName))
            {
                return;
            }

            if (!da.TryGet(
                    2,
                    out string newId))
            {
                return;
            }

            SetCadValues(
                CommandName,
                new { navigatorItemId, newName, newId },
                ToArchicad);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("6be1cc4e-6645-400f-b632-1fccdae0db4d");
    }
}