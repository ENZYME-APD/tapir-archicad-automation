using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Utilities;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class ConnectArchicadComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "IsAlive";

        private const int RefreshAllButtonIndex = 0;
        private const int ExecuteAllButtonIndex = 1;

        public ConnectArchicadComponent()
            : base(
                "ConnectArchicad",
                "Connect to Archicad by port number.",
                GroupNames.General)
        {
            CapsuleButtonTexts =
                new List<string> { "Refresh All", "Execute All" };
        }

        public override void CreateAttributes()
        {
            m_attributes = new ButtonAttributes(
                this,
                2);
        }

        protected override void AddInputs()
        {
            InInteger(
                "Port",
                "Port number.",
                ConnectionSettings.Port);

            InBoolean(
                "Allow Automatic Refresh",
                "Allow Automatic Refresh",
                AutoRefresh);

            InBoolean(
                "Allow Automatic Execution",
                "Allow Automatic Execution",
                AutoExecute);
        }

        protected override void AddOutputs()
        {
            OutText(
                "Success",
                "Sucessful connection to Archicad.");
        }

        protected override void SolveInstance(
            IGH_DataAccess DA)
        {
            Solve(DA);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetItem(
                    0,
                    out int portNumber))
            {
                return;
            }

            if (!da.GetData(
                    1,
                    ref AutoRefresh))
            {
                return;
            }

            if (!da.GetData(
                    2,
                    ref AutoExecute))
            {
                return;
            }

            ConnectionSettings.Port = portNumber;
            var response = SendArchicadCommand(
                CommandName,
                null);

            da.SetData(
                0,
                response.Succeeded);
        }

        public override void OnCapsuleButtonPressed(
            int buttonIndex)
        {
            if (buttonIndex == RefreshAllButtonIndex)
            {
                var doc = OnPingDocument();
                if (doc != null)
                {
                    foreach (var obj in doc.Objects)
                        if (obj is ArchicadAccessorComponent archicadAccessor)
                        {
                            archicadAccessor.ManualRefreshRequested = true;
                            archicadAccessor.ExpireSolution(true);
                            archicadAccessor.ManualRefreshRequested = false;
                        }
                }
            }
            else if (buttonIndex == ExecuteAllButtonIndex)
            {
                var doc = OnPingDocument();
                if (doc != null)
                {
                    foreach (var obj in doc.Objects)
                        if (obj is ArchicadExecutorComponent archicadExecutor)
                        {
                            archicadExecutor.ManualExecuteRequested = true;
                            archicadExecutor.ExpireSolution(true);
                            archicadExecutor.ManualExecuteRequested = false;
                        }
                }
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ConnectArchicad;

        public override Guid ComponentGuid =>
            new Guid("f99302e6-2cbd-438e-ac63-c90f76f4b7d9");
    }
}