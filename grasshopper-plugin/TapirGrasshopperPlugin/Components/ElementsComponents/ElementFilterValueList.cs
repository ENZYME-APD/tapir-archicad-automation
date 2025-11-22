using System;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ElementFilterValueList : ValueList
    {
        public ElementFilterValueList()
            : base(
                "ElementsFilter",
                "Value list for Archicad Elements Filters.",
                GroupNames.Elements)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: ElementFilter.NoFilter);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemFilterValueList;

        public override Guid ComponentGuid =>
            new Guid("642df9ee-ddf9-44e8-98bc-c7fe596dad87");
    }
}