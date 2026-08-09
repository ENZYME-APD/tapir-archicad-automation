using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components
{
    public class AnchorPointValueList : ValueList
    {
        public AnchorPointValueList()
            : base(
                "AnchorPoints",
                "Value list for the cross section anchor point of beams and columns.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: AnchorPoint.Center);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AnchorPointValueList;

        public override Guid ComponentGuid =>
            new Guid("5a3d9b7e-1f2c-4f8b-89f4-6f1d3a0b7c25");
    }
}
