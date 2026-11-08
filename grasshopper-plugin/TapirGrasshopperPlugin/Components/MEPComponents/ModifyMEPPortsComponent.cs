using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class ModifyMEPPortsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyMEPPorts";

        public ModifyMEPPortsComponent()
            : base(
                "ModifyMEPPorts",
                "Modifies the given MEP ports (as returned by GetMEPPorts): shape, connection style (from Archicad 30), " +
                "the flow direction of piping and ventilation ports, and the outer diameter and nominal size " +
                "(from Archicad 30) of piping ports. Only the given fields are changed. Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "PortIds",
                "Identifiers of the ports to modify, as returned by GetMEPPorts.");

            InTexts(
                "Shapes",
                "New shape of each port: Rectangular, Circular, Oval or UShape " +
                "(input only 1 to use the same shape for all). Optional.");

            InTexts(
                "ConnectionStyles",
                "New connection style of each port: SimpleBody, Flanged or Welded " +
                "(input only 1 to use the same style for all; Archicad 30 or newer). Optional.");

            InTexts(
                "FlowDirections",
                "New flow direction of each piping or ventilation port: FlowNotDefined, FlowIn or FlowOut " +
                "(input only 1 to use the same direction for all). Optional.");

            InNumbers(
                "OuterDiameters",
                "New outer diameter of each piping port (input only 1 to use the same diameter for all). Optional.");

            InTexts(
                "NominalSizes",
                "New nominal size of each piping port (input only 1 to use the same size for all; " +
                "Archicad 30 or newer). Optional.");

            SetOptionality(new[] { 1, 2, 3, 4, 5 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each port (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!MEPComponentHelp.TryGetGuidTexts(this, da, 0, out var portIds) ||
                portIds.Count == 0)
            {
                return;
            }

            var fields = new ElectricalItemFields(this, 0, portIds.Count);
            if (!fields.AddTexts(da, 1, "shape") ||
                !fields.AddTexts(da, 2, "connectionStyle") ||
                !fields.AddTexts(da, 3, "flowDirection") ||
                !fields.AddNumbers(da, 4, "outerDiameter") ||
                !fields.AddTexts(da, 5, "nominalSize"))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < portIds.Count; i++)
            {
                var item = new JObject { ["portId"] = portIds[i] };
                fields.ApplyTo(item, i);
                items.Add(item);
            }

            var parameters = new JObject { ["portsData"] = items };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMEPPorts;

        public override Guid ComponentGuid =>
            new Guid("75febf09-0021-4e37-b478-f4b61870cb5b");
    }
}
