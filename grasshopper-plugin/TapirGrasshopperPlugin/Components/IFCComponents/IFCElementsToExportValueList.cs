using System;
using TapirGrasshopperPlugin.Types.Commands;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class IFCElementsToExportValueList : ValueList
    {
        public IFCElementsToExportValueList()
            : base(
                nameof(IFCElementsToExport),
                "Value list for the elements to export with an IFC export " +
                "translator; the ElementsToExport input of IFCFileOperation.",
                GroupNames.IFC)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(
                defaultSelected: IFCElementsToExport.VisibleElementsOnAllStories);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.IFCElementsToExport;

        public override Guid ComponentGuid =>
            new Guid("5d2e8f4a-9b31-47c6-a8e2-3c7f1b9d6a45");
    }
}
