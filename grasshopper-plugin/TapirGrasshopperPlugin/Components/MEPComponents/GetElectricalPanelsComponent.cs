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
    public class GetElectricalPanelsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElectricalPanels";

        public GetElectricalPanelsComponent()
            : base(
                "GetElectricalPanels",
                "Get the electrical panels (distribution boards) of the project or the given ones: name, circuit naming, " +
                "circuits, parent circuit, placement and the connected and demand loads in total and per phase. " +
                "Tree outputs have one branch per panel. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the electrical panels. Optional: every electrical panel of the project when not given.");

            SetOptionality(new[] { 0 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each electrical panel.");

            OutTexts(
                "PanelNames",
                "Name of each panel.");

            OutTexts(
                "CircuitNamePrefixes",
                "Prefix of the names of the circuits of each panel.");

            OutIntegers(
                "CircuitIdStyles",
                "Index of the style of the circuit ids of each panel.");

            OutGenericTree(
                "CircuitGuids",
                "Identifiers of the circuits owned by each panel (one branch per panel).");

            OutGenerics(
                "ParentCircuitGuids",
                "Identifier of the circuit each panel is connected to (null when it is not connected).");

            ElectricalPlacementOutputs.AddOutputs(this);

            OutNumberList(
                "ConnectedLoads",
                "Total connected load of each panel.");

            OutNumbers(
                "ConnectedLoadsPerPhase",
                "Connected load of the phases A, B and C of each panel (one branch per panel).");

            OutNumberList(
                "DemandLoads",
                "Total demand load of each panel.");

            OutNumbers(
                "DemandLoadsPerPhase",
                "Demand load of the phases A, B and C of each panel (one branch per panel).");

            OutErrorMessages(
                "Error message of each panel (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElectricalComponentHelp.TryGetElementIds(
                    this,
                    da,
                    0,
                    out List<ElementGuid> ids))
            {
                return;
            }

            var parameters = new JObject();
            if (ids.Count > 0)
            {
                parameters["elements"] = ElectricalComponentHelp.ElementsArray(ids);
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
            var panelNames = new List<object>();
            var circuitNamePrefixes = new List<object>();
            var circuitIdStyles = new List<object>();
            var circuitGuids = new DataTree<object>();
            var parentCircuitGuids = new List<object>();
            var placements = new ElectricalPlacementOutputs();
            var connectedLoads = new List<object>();
            var connectedLoadsPerPhase = new DataTree<object>();
            var demandLoads = new List<object>();
            var demandLoadsPerPhase = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(response, "electricalPanels");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                circuitGuids.EnsurePath(path);

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
                panelNames.Add(JsonOutputHelp.Scalar(item, "panelName"));
                circuitNamePrefixes.Add(JsonOutputHelp.Scalar(item, "circuitNamePrefix"));
                circuitIdStyles.Add(JsonOutputHelp.Scalar(item, "circuitIdStyle"));
                ElectricalComponentHelp.AddElementIds(item?["circuits"], circuitGuids, path);
                parentCircuitGuids.Add(ElectricalComponentHelp.ElementIdOf(item?["parentCircuitId"]));
                placements.Add(item);
                ElectricalComponentHelp.AddLoads(item?["connectedLoad"], connectedLoads, connectedLoadsPerPhase, path);
                ElectricalComponentHelp.AddLoads(item?["demandLoad"], demandLoads, demandLoadsPerPhase, path);
            }

            da.SetDataList(0, elementGuids);
            da.SetDataList(1, panelNames);
            da.SetDataList(2, circuitNamePrefixes);
            da.SetDataList(3, circuitIdStyles);
            da.SetDataTree(4, circuitGuids);
            da.SetDataList(5, parentCircuitGuids);
            placements.SetData(da, 6);
            var next = 6 + ElectricalPlacementOutputs.Count;
            da.SetDataList(next, connectedLoads);
            da.SetDataTree(next + 1, connectedLoadsPerPhase);
            da.SetDataList(next + 2, demandLoads);
            da.SetDataTree(next + 3, demandLoadsPerPhase);
            da.SetDataList(next + 4, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElectricalPanels;

        public override Guid ComponentGuid =>
            new Guid("56430f4f-3ddb-4c6b-9d70-87d9e1400f03");
    }
}
