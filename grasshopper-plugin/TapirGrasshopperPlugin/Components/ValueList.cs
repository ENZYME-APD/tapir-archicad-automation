using Grasshopper.GUI;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Special;
using Newtonsoft.Json.Linq;
using System;
using System.Drawing;
using System.Windows.Forms;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components
{
    abstract public class ValueList : GH_ValueList
    {
        public ValueList (string name, string nickname, string description, string subCategory)
        {
            CreateAttributes ();

            Name = name;
            NickName = nickname;
            Description = description;
            Category = "Tapir";
            SubCategory = subCategory;

            ListMode = GH_ValueListMode.DropDown;

            RefreshItems ();
        }

        protected CommandResponse SendArchicadCommand (string commandName, JObject commandParameters)
        {
            ArchicadConnection connection = new ArchicadConnection (ConnectionSettings.Port);
            return connection.SendCommand (commandName, commandParameters);
        }

        protected CommandResponse SendArchicadAddOnCommand (string commandNamespace, string commandName, JObject commandParameters)
        {
            ArchicadConnection connection = new ArchicadConnection (ConnectionSettings.Port);
            return connection.SendAddOnCommand (commandNamespace, commandName, commandParameters);
        }

        abstract public void RefreshItems ();

        public void AddAsSource (GH_Component component, int inputIndex)
        {
            if (component.Params.Input.Count <= inputIndex || component.Params.Input[inputIndex].SourceCount > 0) {
                return;
            }

            component.OnPingDocument ().AddObject (this, false);
            Attributes.Pivot = new PointF (
                component.Attributes.Pivot.X + component.Params.Input[inputIndex].Attributes.InputGrip.X - this.Attributes.Bounds.Width * GH_GraphicsUtil.UiScale - 40.0f,
                component.Attributes.Pivot.Y + component.Params.Input[inputIndex].Attributes.InputGrip.Y - this.Attributes.Bounds.Height / 2.0f);
            component.Params.Input[inputIndex].AddSource (this);
            ExpireSolution (true);
        }
    }

    abstract public class ArchicadAccessorValueList : ValueList
    {
        public ArchicadAccessorValueList (string name, string nickname, string description, string subCategory) :
            base (name, nickname, description, subCategory)
        {
        }

        public override void AppendAdditionalMenuItems (ToolStripDropDown menu)
        {
            base.AppendAdditionalMenuItems (menu);
            Menu_AppendItem (menu, "Refresh from Archicad", OnRefreshButtonClicked);
        }

        private void OnRefreshButtonClicked (object sender, EventArgs e)
        {
            RefreshItems ();
        }
    }
}
