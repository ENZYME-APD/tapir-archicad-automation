using System;

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
            base ("Element Filter", "",
                "Value List for Archicad Element Filters.", "Elements")
        {
        }

        public override void RefreshItems ()
        {
            ListItems.Clear ();
            AddEnumItems (defaultSelected: ElementFilter.NoFilter);
            ExpireSolution (true);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ElemFilterValueList;

        public override Guid ComponentGuid => new Guid ("642df9ee-ddf9-44e8-98bc-c7fe596dad87");
    }
}
