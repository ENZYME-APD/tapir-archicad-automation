using System;
using System.Windows.Forms;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public enum MainElementType
    {
        Wall,
        Column,
        Beam,
        Window,
        Door,
        Object,
        Lamp,
        Slab,
        Roof,
        Mesh,
        Dimension,
        RadialDimension,
        LevelDimension,
        AngleDimension,
        Text,
        Label,
        Zone,
        Hatch,
        Line,
        PolyLine,
        Arc,
        Circle,
        Spline,
        Hotspot,
        CutPlane,
        Camera,
        CamSet,
        Group,
        SectElem,
        Drawing,
        Picture,
        Detail,
        Elevation,
        InteriorElevation,
        Worksheet,
        Hotlink,
        CurtainWall,
        Shell,
        Skylight,
        Morph,
        ChangeMarker,
        Stair,
        Railing,
        Opening,
    }

    public enum SubElementType
    {
        CurtainWallSegment,
        CurtainWallFrame,
        CurtainWallPanel,
        CurtainWallJunction,
        CurtainWallAccessory,
        Riser,
        Tread,
        StairStructure,
        RailingToprail,
        RailingHandrail,
        RailingRail,
        RailingPost,
        RailingInnerPost,
        RailingBaluster,
        RailingPanel,
        RailingSegment,
        RailingNode,
        RailingBalusterSet,
        RailingPattern,
        RailingToprailEnd,
        RailingHandrailEnd,
        RailingRailEnd,
        RailingToprailConnection,
        RailingHandrailConnection,
        RailingRailConnection,
        RailingEndFinish,
        BeamSegment,
        ColumnSegment,
    }

    public enum ElementTypeValueListType
    {
        MainElementsOnly,
        SubElementsOnly,
        AllElements
    }

    public class ElementTypeValueList : ValueList
    {
        public static string Doc => "Value List for Archicad Elements Types.";

        private ElementTypeValueListType type;

        public ElementTypeValueList()
            : this(ElementTypeValueListType.AllElements)
        {
        }

        public ElementTypeValueList(
            ElementTypeValueListType t)
            : base(
                "Elements Type",
                "",
                Doc,
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

            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("ce80b380-0e42-467c-865b-69b2e8e5155e");
    }
}