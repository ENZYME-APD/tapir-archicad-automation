using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class ModifyElectricalPanelsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyElectricalPanels";

        private const int PlacementInputsIndex = 5;

        public ModifyElectricalPanelsComponent()
            : base(
                "ModifyElectricalPanels",
                "Modify the name, circuit naming and placement (anchor point, orientation, rotation, mirroring, " +
                "library part, size) of electrical panels. Only the given fields are changed. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the electrical panels to modify.");

            InTexts(
                "PanelNames",
                "New name of each panel (input only 1 to use the same name for all). Optional.");

            InTexts(
                "CircuitNamePrefixes",
                "New prefix of the names of the circuits of each panel (input only 1 to use the same prefix for all). Optional.");

            InIntegers(
                "CircuitIdStyles",
                "New index of the style of the circuit ids of each panel (input only 1 to use the same style for all). Optional.");

            InPoints(
                "AnchorPoints",
                "New anchor point of each panel (input only 1 to use the same point for all). Optional.");

            SetOptionality(new[] { 1, 2, 3, 4 });

            ElectricalPlacementInputs.Add(this, PlacementInputsIndex);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each electrical panel (empty when it was modified successfully).");
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
            if (!fields.AddTexts(da, 1, "panelName") ||
                !fields.AddTexts(da, 2, "circuitNamePrefix") ||
                !fields.AddIntegers(da, 3, "circuitIdStyle") ||
                !fields.AddPoints(da, 4, "anchorPoint") ||
                !ElectricalPlacementInputs.TryRead(fields, da, PlacementInputsIndex))
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

            var parameters = new JObject { ["electricalPanelsData"] = items };

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
            Properties.Resources.ModifyElectricalPanels;

        public override Guid ComponentGuid =>
            new Guid("53d855dc-6ae4-4635-a45f-ae7c7f369679");
    }
}
