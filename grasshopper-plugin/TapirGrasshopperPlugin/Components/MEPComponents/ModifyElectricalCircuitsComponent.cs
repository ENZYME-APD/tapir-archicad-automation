using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class ModifyElectricalCircuitsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyElectricalCircuits";

        public ModifyElectricalCircuitsComponent()
            : base(
                "ModifyElectricalCircuits",
                "Modify the description, voltage, wiring type and used phases of electrical circuits. " +
                "Only the given fields are changed. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the electrical circuits to modify.");

            InTexts(
                "Descriptions",
                "New description of each circuit (input only 1 to use the same description for all). Optional.");

            InNumbers(
                "Voltages",
                "New voltage of each circuit (input only 1 to use the same voltage for all). Optional.");

            InTexts(
                "WiringTypeNames",
                "New wiring type of each circuit: one of the wiring types of the electrical project settings, " +
                "or an empty text for none (input only 1 to use the same wiring type for all). Optional.");

            InBooleans(
                "IsPhaseAUsed",
                "Use phase A in each circuit (input only 1 to use the same value for all). Optional.");

            InBooleans(
                "IsPhaseBUsed",
                "Use phase B in each circuit (input only 1 to use the same value for all). Optional.");

            InBooleans(
                "IsPhaseCUsed",
                "Use phase C in each circuit (input only 1 to use the same value for all). Optional.");

            SetOptionality(new[] { 1, 2, 3, 4, 5, 6 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each circuit (empty when it was modified successfully).");
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

            var fields = new ElectricalItemFields(this, 0, ids.Count);
            if (!fields.AddTexts(da, 1, "description") ||
                !fields.AddNumbers(da, 2, "voltage") ||
                !fields.AddTexts(da, 3, "wiringTypeName") ||
                !fields.AddBooleans(da, 4, "isPhaseAUsed") ||
                !fields.AddBooleans(da, 5, "isPhaseBUsed") ||
                !fields.AddBooleans(da, 6, "isPhaseCUsed"))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < ids.Count; i++)
            {
                var item = new JObject
                {
                    ["elementId"] = new JObject { ["guid"] = ids[i].Guid }
                };
                fields.ApplyTo(item, i);
                items.Add(item);
            }

            var parameters = new JObject { ["electricalCircuitsData"] = items };

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
            Properties.Resources.ModifyElectricalCircuits;

        public override Guid ComponentGuid =>
            new Guid("41e1c8a5-3e97-44d5-be3d-11f16848f8bf");
    }
}
