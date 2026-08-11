using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components
{
    public class ReferenceLineLocationValueList : ValueList
    {
        public ReferenceLineLocationValueList()
            : base(
                "ReferenceLineLocations",
                "Value list for the reference line location of walls.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: ReferenceLineLocation.Center);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ReferenceLineLocationValueList;

        public override Guid ComponentGuid =>
            new Guid("8d1a58f0-6b1e-4b31-9a3a-2e6cbb0a4f77");
    }
}
