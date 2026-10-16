using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class CreateElectricalDevicesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateElectricalDevices";

        private const int PlacementInputsIndex = 5;

        public CreateElectricalDevicesComponent()
            : base(
                "CreateElectricalDevices",
                "Place electrical devices (lights, sockets and other consumers) with the default settings of the " +
                "Electrical Device tool, optionally with the given apparent power, load classification, library part, " +
                "size, orientation, rotation and mirroring, and connect them to a circuit. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InPoints(
                "Positions",
                "Anchor point of each electrical device.");

            InGenerics(
                "CircuitGuids",
                "Identifier of the electrical circuit to connect each device to " +
                "(input only 1 to connect all devices to the same circuit). Optional.");

            InIntegers(
                "ApparentPowers",
                "Apparent power of each device in VA, not negative (input only 1 to use the same power for all). Optional.");

            InBooleans(
                "IsApparentPowerEnabled",
                "Enable the apparent power of each device (input only 1 to use the same value for all). Optional.");

            InTexts(
                "LoadClassificationNames",
                "Load classification of each device: one of the load classifications of the electrical project settings, " +
                "or an empty text for none (input only 1 to use the same classification for all). Optional.");

            SetOptionality(new[] { 1, 2, 3, 4 });

            ElectricalPlacementInputs.Add(this, PlacementInputsIndex);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each created electrical device.");

            OutErrorMessages(
                "Error message of each electrical device (empty when it was created successfully).");
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
            if (!fields.AddElementIds(da, 1, "circuitId") ||
                !fields.AddNonNegativeIntegers(da, 2, "apparentPower") ||
                !fields.AddBooleans(da, 3, "isApparentPowerEnabled") ||
                !fields.AddTexts(da, 4, "loadClassificationName") ||
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

            var parameters = new JObject { ["electricalDevicesData"] = items };

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
            Properties.Resources.CreateElectricalDevices;

        public override Guid ComponentGuid =>
            new Guid("a4b9b5e2-9c74-4881-bc73-cab7a8e1520b");
    }
}
