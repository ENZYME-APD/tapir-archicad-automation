using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class ConnectToElectricalCircuitsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ConnectToElectricalCircuits";

        public ConnectToElectricalCircuitsComponent()
            : base(
                "ConnectToElectricalCircuits",
                "Connect electrical devices, or panels as sub-panels, to electrical circuits. " +
                "A panel connected to a circuit becomes a sub-panel of the panel of that circuit. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the electrical devices or electrical panels to connect.");

            InGenerics(
                "CircuitGuids",
                "Identifier of the electrical circuit to connect each element to " +
                "(input only 1 to connect all elements to the same circuit).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each connection (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElectricalComponentHelp.TryGetElementIds(
                    this,
                    da,
                    0,
                    out List<ElementGuid> ids) ||
                !ElectricalComponentHelp.TryGetElementIds(
                    this,
                    da,
                    1,
                    out List<ElementGuid> circuitIds))
            {
                return;
            }
            if (ids.Count == 0)
            {
                this.AddError("The input ElementGuids must contain at least one item.");
                return;
            }
            if (circuitIds.Count != 1 &&
                circuitIds.Count != ids.Count)
            {
                this.AddError(
                    "The size of the input CircuitGuids must be 1 or equal to the size of the input ElementGuids.");
                return;
            }

            var items = new JArray();
            for (var i = 0; i < ids.Count; i++)
            {
                items.Add(
                    new JObject
                    {
                        ["elementId"] = new JObject { ["guid"] = ids[i].Guid },
                        ["circuitId"] = new JObject
                        {
                            ["guid"] = circuitIds[circuitIds.Count == 1 ? 0 : i].Guid
                        }
                    });
            }

            var parameters = new JObject { ["connectionsData"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response) ||
                ElectricalComponentHelp.ReportResponseError(this, response))
            {
                return;
            }

            da.SetDataList(0, ExecutionResultsResponse.ErrorMessages(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ConnectToElectricalCircuits;

        public override Guid ComponentGuid =>
            new Guid("c4a6db1f-87a3-402e-93e8-81ad45188a07");
    }
}
