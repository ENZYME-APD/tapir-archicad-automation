using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class CreateElectricalPanelsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateElectricalPanels";

        private const int PlacementInputsIndex = 4;

        public CreateElectricalPanelsComponent()
            : base(
                "CreateElectricalPanels",
                "Place electrical panels with the default settings of the Electrical Panel tool, optionally with the " +
                "given panel name, circuit naming, library part, size, orientation, rotation and mirroring. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InPoints(
                "Positions",
                "Anchor point of each electrical panel.");

            InTexts(
                "PanelNames",
                "Name of each panel (input only 1 to use the same name for all). Optional.");

            InTexts(
                "CircuitNamePrefixes",
                "Prefix of the names of the circuits of each panel (input only 1 to use the same prefix for all). Optional.");

            InIntegers(
                "CircuitIdStyles",
                "Index of the style of the circuit ids of each panel (input only 1 to use the same style for all). Optional.");

            SetOptionality(new[] { 1, 2, 3 });

            ElectricalPlacementInputs.Add(this, PlacementInputsIndex);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each created electrical panel.");

            OutErrorMessages(
                "Error message of each electrical panel (empty when it was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<Point3d> positions))
            {
                return;
            }

            var fields = new ElectricalItemFields(this, 0, positions.Count);
            if (!fields.AddTexts(da, 1, "panelName") ||
                !fields.AddTexts(da, 2, "circuitNamePrefix") ||
                !fields.AddIntegers(da, 3, "circuitIdStyle") ||
                !ElectricalPlacementInputs.TryRead(fields, da, PlacementInputsIndex))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < positions.Count; i++)
            {
                var item = new JObject
                {
                    ["position"] = ElectricalComponentHelp.Coordinate(positions[i])
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

            SetCreatedIdsAndErrorMessages<ElementGuid>(
                da,
                response,
                "elements",
                "elementId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateElectricalPanels;

        public override Guid ComponentGuid =>
            new Guid("0645e186-402b-4ce7-b3e1-1f6cf3d1fc80");
    }
}
