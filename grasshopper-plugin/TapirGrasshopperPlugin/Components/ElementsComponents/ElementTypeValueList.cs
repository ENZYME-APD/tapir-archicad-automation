using Grasshopper.Kernel;
using Grasshopper.Kernel.Special;
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

    public class ElementTypeValueList : GH_ValueList
    {
        private bool mainElementsOnly;

        public ElementTypeValueList ()
        {
            CreateAttributes ();

            Name = "Element Type";
            NickName = "ElemType";
            Description = "Value List for Archicad Element Types.";
            Category = "Tapir";
            SubCategory = "Elements";

            ListMode = GH_ValueListMode.DropDown;

            mainElementsOnly = true;

            AddItems ();
        }

        public override void AppendAdditionalMenuItems (ToolStripDropDown menu)
        {
            menu.Items.Clear ();
            GH_DocumentObject.Menu_AppendItem (menu, "MainElements", Menu_MainElementsClicked, enabled: true, @checked: mainElementsOnly);
            GH_DocumentObject.Menu_AppendItem (menu, "SubElements", Menu_SubElementsClicked, enabled: true, @checked: !mainElementsOnly);
        }

        private void Menu_MainElementsClicked (object sender, EventArgs e)
        {
            mainElementsOnly = true;
            AddItems ();
        }

        private void Menu_SubElementsClicked (object sender, EventArgs e)
        {
            mainElementsOnly = false;
            AddItems ();
        }

        private void AddItems ()
        {
            ListItems.Clear ();
            if (mainElementsOnly) {
                foreach (MainElementType type in Enum.GetValues (typeof (MainElementType))) {
                    var item = new GH_ValueListItem (type.ToString (), '"' + type.ToString () + '"');
                    item.Selected = type == MainElementType.Wall;
                    ListItems.Add (item);
                }
            } else {
                foreach (SubElementType type in Enum.GetValues (typeof (SubElementType))) {
                    var item = new GH_ValueListItem (type.ToString (), '"' + type.ToString () + '"');
                    item.Selected = type == SubElementType.CurtainWallPanel;
                    ListItems.Add (item);
                }
            }
            ExpireSolution (true);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ElemType;

        public override Guid ComponentGuid => new Guid ("6e504f2a-c7a1-4b6f-8002-f6ad19f49a2e");
    }
}
