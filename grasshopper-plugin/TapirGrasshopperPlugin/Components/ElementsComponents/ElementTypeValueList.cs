using System;
using System.Windows.Forms;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ElementTypeValueList : ValueList
    {
        private ElementTypeValueListType type;

        public ElementTypeValueList()
            : this(ElementTypeValueListType.AllElements)
        {
        }

        public ElementTypeValueList(
            ElementTypeValueListType t)
            : base(
                "ElementsType",
                "Value List for Archicad Elements Types.",
                GroupNames.Elements)
        {
            type = t;
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
            switch (type)
            {
                case ElementTypeValueListType.SubElementsOnly:
                    type = ElementTypeValueListType.AllElements;
                    break;
                case ElementTypeValueListType.AllElements:
                    type = ElementTypeValueListType.SubElementsOnly;
                    break;
                default:
                    return;
            }

            RefreshItems();
        }

        private void Menu_SubElementsClicked(
            object sender,
            EventArgs e)
        {
            switch (type)
            {
                case ElementTypeValueListType.MainElementsOnly:
                    type = ElementTypeValueListType.AllElements;
                    break;
                case ElementTypeValueListType.AllElements:
                    type = ElementTypeValueListType.MainElementsOnly;
                    break;
                default:
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

            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("ce80b380-0e42-467c-865b-69b2e8e5155e");
    }
}