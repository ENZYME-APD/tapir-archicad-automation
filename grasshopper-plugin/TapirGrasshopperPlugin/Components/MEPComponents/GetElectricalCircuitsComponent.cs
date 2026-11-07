using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetElectricalCircuitsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElectricalCircuits";

        public GetElectricalCircuitsComponent()
            : base(
                "GetElectricalCircuits",
                "Get the given electrical circuits, the circuits of the given panels, or every circuit of the project: " +
                "name, description, owner panel, connected devices and panels, voltage, phases, wiring type, loads, " +
                "voltage drop and estimated cable length. The outputs list the given circuits first, then the circuits " +
                "of the given panels. Tree outputs have one branch per circuit. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the electrical circuits. Optional.");

            InGenerics(
                "PanelGuids",
                "Identifiers of the electrical panels whose circuits are listed. Optional: when neither this nor " +
                "ElementGuids is given, every circuit of the project is listed.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each circuit.");

            OutTexts(
                "Names",
                "Name of each circuit, including the circuit name prefix of its panel.");

            OutTexts(
                "Descriptions",
                "Description of each circuit.");

            OutGenerics(
                "OwnerPanelGuids",
                "Identifier of the panel that owns each circuit.");

            OutGenericTree(
                "DeviceGuids",
                "Identifiers of the electrical devices connected to each circuit (one branch per circuit).");

            OutGenericTree(
                "SubPanelGuids",
                "Identifiers of the electrical panels connected to each circuit (one branch per circuit).");

            OutNumberList(
                "Voltages",
                "Voltage of each circuit.");

            OutTexts(
                "Phases",
                "Used phases of each circuit, as Archicad displays them.");

            OutBooleans(
                "IsPhaseAUsed",
                "True if the circuit uses phase A.");

            OutBooleans(
                "IsPhaseBUsed",
                "True if the circuit uses phase B.");

            OutBooleans(
                "IsPhaseCUsed",
                "True if the circuit uses phase C.");

            OutTexts(
                "WiringTypeNames",
                "Name of the wiring type of each circuit, as defined in the electrical project settings.");

            OutNumberList(
                "ConnectedLoads",
                "Total connected load of each circuit.");

            OutNumbers(
                "ConnectedLoadsPerPhase",
                "Connected load of the phases A, B and C of each circuit (one branch per circuit).");

            OutNumberList(
                "DemandLoads",
                "Total demand load of each circuit.");

            OutNumbers(
                "DemandLoadsPerPhase",
                "Demand load of the phases A, B and C of each circuit (one branch per circuit).");

            OutNumberList(
                "VoltageDrops",
                "Voltage drop of each circuit.");

            OutNumberList(
                "EstimatedCableLengths",
                "Estimated cable length of each circuit.");

            OutErrorMessages(
                "Error message of each circuit or failed panel (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElectricalComponentHelp.TryGetElementIds(
                    this,
                    da,
                    0,
                    out List<ElementGuid> circuitIds) ||
                !ElectricalComponentHelp.TryGetElementIds(
                    this,
                    da,
                    1,
                    out List<ElementGuid> panelIds))
            {
                return;
            }

            var parameters = new JObject();
            if (circuitIds.Count > 0)
            {
                parameters["elements"] = ElectricalComponentHelp.ElementsArray(circuitIds);
            }
            if (panelIds.Count > 0)
            {
                parameters["electricalPanels"] = ElectricalComponentHelp.ElementsArray(panelIds);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response) ||
                ElectricalComponentHelp.ReportResponseError(this, response))
            {
                return;
            }

            var elementGuids = new List<object>();
            var names = new List<object>();
            var descriptions = new List<object>();
            var ownerPanelGuids = new List<object>();
            var deviceGuids = new DataTree<object>();
            var subPanelGuids = new DataTree<object>();
            var voltages = new List<object>();
            var phases = new List<object>();
            var isPhaseAUsed = new List<object>();
            var isPhaseBUsed = new List<object>();
            var isPhaseCUsed = new List<object>();
            var wiringTypeNames = new List<object>();
            var connectedLoads = new List<object>();
            var connectedLoadsPerPhase = new DataTree<object>();
            var demandLoads = new List<object>();
            var demandLoadsPerPhase = new DataTree<object>();
            var voltageDrops = new List<object>();
            var cableLengths = new List<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(response, "electricalCircuits");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                deviceGuids.EnsurePath(path);
                subPanelGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    item = null;
                    errors.Add(JsonOutputHelp.ErrorMessage(items[i]));
                }
                else
                {
                    errors.Add("");
                }

                elementGuids.Add(ElectricalComponentHelp.ElementIdOf(item?["elementId"]));
                names.Add(JsonOutputHelp.Scalar(item, "name"));
                descriptions.Add(JsonOutputHelp.Scalar(item, "description"));
                ownerPanelGuids.Add(ElectricalComponentHelp.ElementIdOf(item?["ownerPanelId"]));
                ElectricalComponentHelp.AddElementIds(item?["childDevices"], deviceGuids, path);
                ElectricalComponentHelp.AddElementIds(item?["childPanels"], subPanelGuids, path);
                voltages.Add(JsonOutputHelp.Scalar(item, "voltage"));
                phases.Add(JsonOutputHelp.Scalar(item, "phases"));
                isPhaseAUsed.Add(JsonOutputHelp.Scalar(item, "isPhaseAUsed"));
                isPhaseBUsed.Add(JsonOutputHelp.Scalar(item, "isPhaseBUsed"));
                isPhaseCUsed.Add(JsonOutputHelp.Scalar(item, "isPhaseCUsed"));
                wiringTypeNames.Add(JsonOutputHelp.Scalar(item, "wiringTypeName"));
                ElectricalComponentHelp.AddLoads(item?["connectedLoad"], connectedLoads, connectedLoadsPerPhase, path);
                ElectricalComponentHelp.AddLoads(item?["demandLoad"], demandLoads, demandLoadsPerPhase, path);
                voltageDrops.Add(JsonOutputHelp.Scalar(item, "voltageDrop"));
                cableLengths.Add(JsonOutputHelp.Scalar(item, "estimatedCableLength"));
            }

            da.SetDataList(0, elementGuids);
            da.SetDataList(1, names);
            da.SetDataList(2, descriptions);
            da.SetDataList(3, ownerPanelGuids);
            da.SetDataTree(4, deviceGuids);
            da.SetDataTree(5, subPanelGuids);
            da.SetDataList(6, voltages);
            da.SetDataList(7, phases);
            da.SetDataList(8, isPhaseAUsed);
            da.SetDataList(9, isPhaseBUsed);
            da.SetDataList(10, isPhaseCUsed);
            da.SetDataList(11, wiringTypeNames);
            da.SetDataList(12, connectedLoads);
            da.SetDataTree(13, connectedLoadsPerPhase);
            da.SetDataList(14, demandLoads);
            da.SetDataTree(15, demandLoadsPerPhase);
            da.SetDataList(16, voltageDrops);
            da.SetDataList(17, cableLengths);
            da.SetDataList(18, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElectricalCircuits;

        public override Guid ComponentGuid =>
            new Guid("3fbac47a-58e8-4c79-8370-05405779766a");
    }
}
