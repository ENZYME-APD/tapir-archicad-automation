using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components
{
    public class ReferencePlaneLocationValueList : ValueList
    {
        public ReferencePlaneLocationValueList()
            : base(
                "ReferencePlaneLocations",
                "Value list for the reference plane location of slabs.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: ReferencePlaneLocation.Top);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ReferencePlaneLocationValueList;

        public override Guid ComponentGuid =>
            new Guid("bf4a1de4-3d59-4a67-9c65-c3a5cf9e0a12");
    }
}
