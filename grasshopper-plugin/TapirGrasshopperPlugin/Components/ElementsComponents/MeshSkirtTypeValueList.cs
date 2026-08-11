using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components
{
    public class MeshSkirtTypeValueList : ValueList
    {
        public MeshSkirtTypeValueList()
            : base(
                "MeshSkirtTypes",
                "Value list for the skirt type of mesh elements.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: MeshSkirtType.WithSkirt);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MeshSkirtTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("9f6c2a54-8e17-4c0d-b3a2-71d5c8e4f096");
    }
}
