using Grasshopper.Kernel;
using Grasshopper.Kernel.Special;
using Grasshopper.GUI;
using System;
using System.Drawing;
using System.Windows.Forms;
using System.Runtime.CompilerServices;
using System.Collections.Generic;
using Newtonsoft.Json.Linq;

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
        public NavigatorTreeTypeValueList () :
            base ("Navigator Tree Types", "", "Value List for Navigator Tree Types.", "Navigator")
        {
        }

        public override void RefreshItems ()
        {
            ListItems.Clear ();
            AddEnumItems (defaultSelected: NavigatorTreeType.ProjectMap);
            ExpireSolution (true);
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.NavigatorTreeTypeValueList;

        public override Guid ComponentGuid => new Guid ("71f27bd1-bffb-4870-b5a7-cdc0bde25ddb");
    }
}
