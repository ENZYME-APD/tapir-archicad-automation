using System;
using TapirGrasshopperPlugin.ResponseTypes.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class NavigatorTreeTypeValueList : ValueList
    {
        public NavigatorTreeTypeValueList()
            : base(
                "NavigatorTreeTypes",
                "Value List for Navigator Tree Types.",
                GroupNames.Navigator)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: NavigatorTreeType.ProjectMap);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.NavigatorTreeTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("71f27bd1-bffb-4870-b5a7-cdc0bde25ddb");
    }
}