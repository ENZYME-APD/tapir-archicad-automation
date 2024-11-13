using Grasshopper.Kernel;
using Grasshopper.Kernel.Special;
using Grasshopper.GUI;
using System;
using System.Drawing;
using System.Windows.Forms;
using System.Runtime.CompilerServices;

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
        private ElementTypeValueListType type;

        public ElementTypeValueList () :
            this (ElementTypeValueListType.AllElements)
        {
        }

        public ElementTypeValueList (ElementTypeValueListType t) :
            base ("Element Type", "", "Value List for Archicad Element Types.", "Elements")
        {
            type = t;
        }

        public override void AppendAdditionalMenuItems (ToolStripDropDown menu)
        {
            menu.Items.Clear ();
            GH_DocumentObject.Menu_AppendItem (menu, "MainElements", Menu_MainElementsClicked, enabled: true, @checked: type != ElementTypeValueListType.SubElementsOnly);
            GH_DocumentObject.Menu_AppendItem (menu, "SubElements", Menu_SubElementsClicked, enabled: true, @checked: type != ElementTypeValueListType.MainElementsOnly);
        }

        private void Menu_MainElementsClicked (object sender, EventArgs e)
        {
            if (type == ElementTypeValueListType.SubElementsOnly) {
                type = ElementTypeValueListType.AllElements;
            } else if (type == ElementTypeValueListType.AllElements) {
                type = ElementTypeValueListType.SubElementsOnly;
            } else {
                return;
            }
            RefreshItems ();
        }

        private void Menu_SubElementsClicked (object sender, EventArgs e)
        {
            if (type == ElementTypeValueListType.MainElementsOnly) {
                type = ElementTypeValueListType.AllElements;
            } else if (type == ElementTypeValueListType.AllElements) {
                type = ElementTypeValueListType.MainElementsOnly;
            } else {
                return;
            }
            RefreshItems ();
        }

        public override void RefreshItems ()
        {
            ListItems.Clear ();
            if (type != ElementTypeValueListType.SubElementsOnly) {
                foreach (MainElementType type in Enum.GetValues (typeof (MainElementType))) {
                    var item = new GH_ValueListItem (type.ToString (), '"' + type.ToString () + '"');
                    item.Selected = type == MainElementType.Wall;
                    ListItems.Add (item);
                }
            }
            if (type != ElementTypeValueListType.MainElementsOnly) {
                foreach (SubElementType type in Enum.GetValues (typeof (SubElementType))) {
                    var item = new GH_ValueListItem (type.ToString (), '"' + type.ToString () + '"');
                    item.Selected = type == SubElementType.CurtainWallPanel;
                    ListItems.Add (item);
                }
            }
            ExpireSolution (true);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ElemType;

        public override Guid ComponentGuid => new Guid ("ce80b380-0e42-467c-865b-69b2e8e5155e");
    }
}
