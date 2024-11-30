using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.General
{
    public class ConnectArchicadComponent : ArchicadAccessorComponent
    {
        public ConnectArchicadComponent ()
          : base (
                "Connect Archicad",
                "ConnectArchicad",
                "Connect to Archicad by port number.",
                "General"
            )
        {
            CapsuleButtonText = "Refresh All";
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddIntegerParameter ("Port", "Port", "Port number", GH_ParamAccess.item, @default: ConnectionSettings.Port);
            pManager.AddBooleanParameter ("Allow Automatic Refresh", "Auto Refresh", "Allow Automatic Refresh", GH_ParamAccess.item, @default: AutoRefresh);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter ("Success", "Success", "Sucessful connection to Archicad.", GH_ParamAccess.item);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            Solve (DA);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            int portNumber = 0;
            if (!DA.GetData (0, ref portNumber)) {
                return;
            }

            if (!DA.GetData (1, ref AutoRefresh)) {
                return;
            }

            ConnectionSettings.Port = portNumber;
            CommandResponse response = SendArchicadCommand ("IsAlive", null);
            DA.SetData (0, response.Succeeded);
        }

        public override void OnCapsuleButtonPressed ()
        {
            ManualRefreshRequested = true;
            try {
                GH_Document doc = OnPingDocument ();
                if (doc != null) {
                    foreach (IGH_DocumentObject obj in doc.Objects)
                        if (obj is ArchicadAccessorComponent archicadAccessor) {
                            archicadAccessor.ExpireSolution (true);
                        }
                }
            } finally {
                ManualRefreshRequested = false;
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ConnectArchicad;

        public override Guid ComponentGuid => new Guid ("f99302e6-2cbd-438e-ac63-c90f76f4b7d9");
    }
}
