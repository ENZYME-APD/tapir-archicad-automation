using Grasshopper.Kernel;
using System;
using Tapir.Utilities;

namespace Tapir.Components
{
    public class ConnectArchicadComponent : Component
    {
        public ConnectArchicadComponent ()
          : base (
                "Connect to Archicad by port number",
                "ConnectArchicad",
                "Connect to Archicad by port number.",
                "General"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddIntegerParameter ("Port", "Port", "Port number", GH_ParamAccess.item, ConnectionSettings.Port);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter ("Success", "Success", "Sucessful connection to Archicad.", GH_ParamAccess.item);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            int portNumber = 0;
            if (!DA.GetData (0, ref portNumber)) {
                return;
            }

            ConnectionSettings.Port = portNumber;
            CommandResponse response = SendArchicadCommand ("IsAlive", null);
            DA.SetData (0, response.Succeeded);
        }

        protected override System.Drawing.Bitmap Icon => Tapir.Properties.Resources.TapirLogo;

        public override Guid ComponentGuid => new Guid ("f99302e6-2cbd-438e-ac63-c90f76f4b7d9");
    }
}
