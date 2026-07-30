using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class SpecialFolderTypeValueList : ValueList
    {
        public SpecialFolderTypeValueList()
            : base(
                "SpecialFolderTypes",
                "Value list for Archicad special folder types.",
                GroupNames.General)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: SpecialFolderType.UserDocuments);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SpecialFolderTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("f0f622d5-3655-4cdc-abf6-90afc42ce137");
    }
}
