using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Windows.Forms;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components
{
    abstract public class Component : GH_Component
    {
        public Component (string name, string nickname, string description, string subCategory) :
            base (name, nickname, description, "Tapir", subCategory)
        {
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
    }

    abstract public class ArchicadAccessorComponent : Component
    {
        public ArchicadAccessorComponent (string name, string nickname, string description, string subCategory) :
            base (name, nickname, description, subCategory)
        {
        }

        protected override void AppendAdditionalComponentMenuItems (ToolStripDropDown menu)
        {
            base.AppendAdditionalComponentMenuItems (menu);
            Menu_AppendItem (menu, "Refresh from Archicad", OnRefreshButtonClicked);
        }

        private void OnRefreshButtonClicked (object sender, EventArgs e)
        {
            ExpireSolution (true);
        }
    }
}
