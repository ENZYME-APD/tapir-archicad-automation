using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components
{
    public class StructureTypeValueList : ValueList
    {
        public StructureTypeValueList()
            : base(
                "StructureTypes",
                "Value list for the structure type of walls, slabs and roofs.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: StructureType.Basic);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.StructureTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("2b5c1c04-06f6-4d0e-9a2e-6f6f3d51c5a1");
    }
}
