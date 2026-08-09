using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components
{
    public class MeshRidgeTypeValueList : ValueList
    {
        public MeshRidgeTypeValueList()
            : base(
                "MeshRidgeTypes",
                "Value list for the ridge type of mesh elements.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: MeshRidgeType.AllSharp);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MeshRidgeTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("1c8f47b9-52a6-4e3f-9d10-4b6e2f8a53c4");
    }
}
