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

    public class ElementTypeValueList : GH_ValueList
    {
        private ElementTypeValueListType type;

        public ElementTypeValueList () : this (ElementTypeValueListType.AllElements)
        {
        }

        private ElementTypeValueList (ElementTypeValueListType t)
        {
            CreateAttributes ();

            Name = "Element Type";
            NickName = "";
            Description = "Value List for Archicad Element Types.";
            Category = "Tapir";
            SubCategory = "Elements";

            ListMode = GH_ValueListMode.DropDown;

            type = t;

            AddItems ();
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
            AddItems ();
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
            AddItems ();
        }

        private void AddItems ()
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

        static public void AddAsSource (GH_Component component, int inputIndex, ElementTypeValueListType type)
        {
            if (component.Params.Input.Count <= inputIndex) {
                return;
            }

            var vallist = new ElementTypeValueList (type);
            vallist.CreateAttributes ();
            component.OnPingDocument ().AddObject (vallist, false);
            vallist.Attributes.Pivot = new PointF (
                component.Attributes.Pivot.X + component.Params.Input[inputIndex].Attributes.InputGrip.X - vallist.Attributes.Bounds.Width * GH_GraphicsUtil.UiScale - 40.0f,
                component.Attributes.Pivot.Y + component.Params.Input[inputIndex].Attributes.InputGrip.Y - vallist.Attributes.Bounds.Height / 2.0f);
            component.Params.Input[inputIndex].AddSource (vallist);
            vallist.ExpireSolution (true);
        }

        public override Guid ComponentGuid => new Guid ("ce80b380-0e42-467c-865b-69b2e8e5155e");
    }
}
