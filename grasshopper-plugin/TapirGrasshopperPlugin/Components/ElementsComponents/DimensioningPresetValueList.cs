using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components
{
    public class DimensioningPresetValueList : ValueList
    {
        public DimensioningPresetValueList()
            : base(
                "DimensioningPresets",
                "Value list for the preset of associative dimensions on section databases.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: DimensioningPreset.WallCompositeFaces);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DimensioningPresetValueList;

        public override Guid ComponentGuid =>
            new Guid("6e2b0d3f-7c94-4a15-8f6b-0a3d9c5e72b8");
    }
}
