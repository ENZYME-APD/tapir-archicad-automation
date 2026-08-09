using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components
{
    public class MEPDomainValueList : ValueList
    {
        public MEPDomainValueList()
            : base(
                "MEPDomains",
                "Value list for the MEP domain of the segment preference tables.",
                GroupNames.MEP)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: MEPDomain.Piping);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MEPDomainValueList;

        public override Guid ComponentGuid =>
            new Guid("3a9d6f10-b845-4c72-9e03-5d1f7b6a4e29");
    }
}
