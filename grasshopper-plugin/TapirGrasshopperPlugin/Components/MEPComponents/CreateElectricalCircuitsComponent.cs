using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class CreateElectricalCircuitsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateElectricalCircuits";

        public CreateElectricalCircuitsComponent()
            : base(
                "CreateElectricalCircuits",
                "Add new circuits to electrical panels, optionally with the given description, voltage, wiring type " +
                "and used phases. Each item adds one circuit to the given panel. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PanelGuids",
                "Identifier of the electrical panel to add each new circuit to " +
                "(the same panel can be given several times to add several circuits to it).");

            InTexts(
                "Descriptions",
                "Description of each circuit (input only 1 to use the same description for all). Optional.");

            InNumbers(
                "Voltages",
                "Voltage of each circuit (input only 1 to use the same voltage for all). Optional.");

            InTexts(
                "WiringTypeNames",
                "Wiring type of each circuit: one of the wiring types of the electrical project settings, " +
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
            OutGenerics(
                "ElementGuids",
                "Identifier of each created circuit.");

            OutErrorMessages(
                "Error message of each circuit (empty when it was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElectricalComponentHelp.TryGetElementIds(
                    this,
                    da,
                    0,
                    out List<ElementGuid> panelIds))
            {
                return;
            }
            if (panelIds.Count == 0)
            {
                this.AddError("The input PanelGuids must contain at least one item.");
                return;
            }

            var fields = new ElectricalItemFields(this, 0, panelIds.Count);
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
            for (var i = 0; i < panelIds.Count; i++)
            {
                var item = new JObject
                {
                    ["electricalPanelId"] = new JObject { ["guid"] = panelIds[i].Guid }
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

            SetCreatedIdsAndErrorMessages<ElementGuid>(
                da,
                response,
                "elements",
                "elementId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateElectricalCircuits;

        public override Guid ComponentGuid =>
            new Guid("7a8989f1-b123-4c44-bd9e-fe6170575e59");
    }
}
