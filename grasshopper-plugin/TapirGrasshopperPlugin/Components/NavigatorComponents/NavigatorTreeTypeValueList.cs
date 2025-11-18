using Eto.Forms;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public enum NavigatorTreeType
    {
        ProjectMap,
        ViewMap,
        LayoutBook,
        PublisherSets
    }

    public class NavigatorTreeTypeValueList : ValueList
    {
        public static string Doc => "Value List for Navigator Tree Types.";

        public NavigatorTreeTypeValueList()
            : base(
                "Navigator Tree Types",
                "",
                Doc,
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