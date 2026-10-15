using System;
using System.Windows.Forms;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ElementTypeValueList : ValueList
    {
        private ElementTypeValueListType type;

        // The item selected when the list is created; null keeps the first
        // item of each list (Wall, CurtainWallPanel).
        private readonly string defaultSelected;

        public ElementTypeValueList()
            : this(ElementTypeValueListType.AllElements)
        {
        }

        public ElementTypeValueList(
            ElementTypeValueListType t,
            string defaultSelected = null)
            : base(
                "ElementsType",
                "Value List for Archicad Elements Types.",
                GroupNames.Elements)
        {
            type = t;
            this.defaultSelected = defaultSelected;

            // The base constructor already filled the list, but it ran before
            // the fields above were set (so with the default MainElementsOnly
            // type and no default selection); fill it again with them.
            RefreshItems();
        }

        public override void AppendAdditionalMenuItems(
            ToolStripDropDown menu)
        {
            menu.Items.Clear();

            Menu_AppendItem(
                menu,
                "MainElements",
                Menu_MainElementsClicked,
                enabled: true,
                @checked: type != ElementTypeValueListType.SubElementsOnly);

            Menu_AppendItem(
                menu,
                "SubElements",
                Menu_SubElementsClicked,
                enabled: true,
                @checked: type != ElementTypeValueListType.MainElementsOnly);
        }

        private void Menu_MainElementsClicked(
            object sender,
            EventArgs e)
        {
            if (type == ElementTypeValueListType.SubElementsOnly)
            {
                type = ElementTypeValueListType.AllElements;
            }
            else if (type == ElementTypeValueListType.AllElements)
            {
                type = ElementTypeValueListType.SubElementsOnly;
            }
            else
            {
                return;
            }

            RefreshItems();
        }

        private void Menu_SubElementsClicked(
            object sender,
            EventArgs e)
        {
            if (type == ElementTypeValueListType.MainElementsOnly)
            {
                type = ElementTypeValueListType.AllElements;
            }
            else if (type == ElementTypeValueListType.AllElements)
            {
                type = ElementTypeValueListType.MainElementsOnly;
            }
            else
            {
                return;
            }

            RefreshItems();
        }

        public override void RefreshItems()
        {
            ListItems.Clear();

            if (type != ElementTypeValueListType.SubElementsOnly)
            {
                AddEnumItems(defaultSelected: MainElementType.Wall);
            }

            if (type != ElementTypeValueListType.MainElementsOnly)
            {
                AddEnumItems(defaultSelected: SubElementType.CurtainWallPanel);
            }

            if (defaultSelected != null)
            {
                foreach (var item in ListItems)
                {
                    item.Selected = item.Name == defaultSelected;
                }
            }

            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("ce80b380-0e42-467c-865b-69b2e8e5155e");
    }
}
