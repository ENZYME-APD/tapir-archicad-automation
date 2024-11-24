using Grasshopper.Kernel;
using Grasshopper.Kernel.Special;
using Grasshopper.GUI;
using System;
using System.Drawing;
using System.Windows.Forms;
using System.Runtime.CompilerServices;
using System.Collections.Generic;
using Newtonsoft.Json.Linq;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public enum ElementFilter
    {
        NoFilter,
        IsEditable,
        IsVisibleByLayer,
        IsVisibleByRenovation,
        IsVisibleByStructureDisplay,
        IsVisibleIn3D,
        OnActualFloor,
        OnActualLayout,
        InMyWorkspace,
        IsIndependent,
        InCroppedView,
        HasAccessRight,
        IsOverriddenByRenovation
    }

    public class ElementFilterValueList : ValueList
    {
        public ElementFilterValueList () :
            base ("Element Filter", "", "Value List for Archicad Element Filters.", "Elements")
        {
        }

        public override void RefreshItems ()
        {
            ListItems.Clear ();
            AddEnumItems (defaultSelected: ElementFilter.NoFilter);
            ExpireSolution (true);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ElemFilter;

        public override Guid ComponentGuid => new Guid ("642df9ee-ddf9-44e8-98bc-c7fe596dad87");
    }
}
