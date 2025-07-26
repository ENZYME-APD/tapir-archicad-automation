using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Utilities;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.General
{
    public class ConnectArchicadComponent : ArchicadAccessorComponent
    {
        private const int RefreshAllButtonIndex = 0;
        private const int ExecuteAllButtonIndex = 1;

        public ConnectArchicadComponent ()
          : base (
                "Connect Archicad",
                "ConnectArchicad",
                "Connect to Archicad by port number.",
                "General"
            )
        {
            CapsuleButtonTexts = new List<string> () {
                "Refresh All",
                "Execute All"
            };
        }

        public override void CreateAttributes ()
        {
            this.m_attributes = new ButtonAttributes (this, 2);
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddIntegerParameter ("Port", "Port", "Port number", GH_ParamAccess.item, @default: ConnectionSettings.Port);
            pManager.AddBooleanParameter ("Allow Automatic Refresh", "Auto Refresh", "Allow Automatic Refresh", GH_ParamAccess.item, @default: AutoRefresh);
            pManager.AddBooleanParameter ("Allow Automatic Execution", "Auto Execute", "Allow Automatic Execution", GH_ParamAccess.item, @default: AutoExecute);
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

            if (!DA.GetData (2, ref AutoExecute)) {
                return;
            }

            ConnectionSettings.Port = portNumber;
            CommandResponse response = SendArchicadCommand ("IsAlive", null);
            DA.SetData (0, response.Succeeded);
        }

        public override void OnCapsuleButtonPressed (int buttonIndex)
        {
            if (buttonIndex == RefreshAllButtonIndex) {
                GH_Document doc = OnPingDocument ();
                if (doc != null) {
                    foreach (IGH_DocumentObject obj in doc.Objects)
                        if (obj is ArchicadAccessorComponent archicadAccessor) {
                            archicadAccessor.ManualRefreshRequested = true;
                            archicadAccessor.ExpireSolution (true);
                            archicadAccessor.ManualRefreshRequested = false;
                        }
                }
            } else if (buttonIndex == ExecuteAllButtonIndex) {
                GH_Document doc = OnPingDocument ();
                if (doc != null) {
                    foreach (IGH_DocumentObject obj in doc.Objects)
                        if (obj is ArchicadExecutorComponent archicadExecutor) {
                            archicadExecutor.ManualExecuteRequested = true;
                            archicadExecutor.ExpireSolution (true);
                            archicadExecutor.ManualExecuteRequested = false;
                        }
                }
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ConnectArchicad;

        public override Guid ComponentGuid => new Guid ("f99302e6-2cbd-438e-ac63-c90f76f4b7d9");
    }
}
