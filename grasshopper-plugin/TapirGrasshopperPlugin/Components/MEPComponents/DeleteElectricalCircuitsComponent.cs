using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class DeleteElectricalCircuitsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteElectricalCircuits";

        public DeleteElectricalCircuitsComponent()
            : base(
                "DeleteElectricalCircuits",
                "Remove electrical circuits from their panels. Removing a circuit disconnects its devices and sub-panels. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the electrical circuits to remove.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each circuit (empty when it was removed successfully).");
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
            if (ids.Count == 0)
            {
                this.AddError("The input ElementGuids must contain at least one item.");
                return;
            }

            var parameters = new JObject
            {
                ["electricalCircuits"] = ElectricalComponentHelp.ElementsArray(ids)
            };

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
            Properties.Resources.DeleteElectricalCircuits;

        public override Guid ComponentGuid =>
            new Guid("d92bbb9f-99a5-45fc-9d6d-d7bdf5c0e450");
    }
}
