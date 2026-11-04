using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class CreateMEPTakeOffsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateMEPTakeOffs";

        public CreateMEPTakeOffsComponent()
            : base(
                "CreateMEPTakeOffs",
                "Creates MEP take-offs on the given routing segments, at a distance from the beginning of the " +
                "segment and optionally rotated around its axis by a multiple of pi/2, with the current settings " +
                "of the Take-Off tool of their domain, optionally with their length and without insulation. " +
                "Archicad only has Ventilation take-offs. Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "SegmentGuids",
                "The routing segment to place each take-off on (one of the segments GetMEPRoutingElements returns).");

            InTexts(
                "Domains",
                "MEP domain of the Take-Off tool settings used for each take-off: Ventilation, Piping or CableCarrier " +
                "(input only 1 to use the same domain for all). Optional; the domain of the segment's routing " +
                "element when it is omitted.");

            InNumbers(
                "DistancesFromBeginning",
                "Distance of each take-off from the beginning of its segment in meters, not negative and not more " +
                "than the length of the segment (input only 1 to use the same distance for all).");

            InNumbers(
                "RotationsAroundMainAxis",
                "Rotation of each take-off around the axis of its segment, in radians, a multiple of pi/2 because " +
                "Archicad places take-offs only at quarter turns: 0 faces left relative to the segment's direction, " +
                "pi/2 up, pi right, 3*pi/2 down (input only 1 to use the same rotation for all). " +
                "Optional; 0 when it is omitted.");

            InNumbers(
                "Lengths",
                "Length of each take-off in meters, positive, set after placing it (input only 1 to use the same " +
                "length for all). Optional; the length of the Take-Off tool when it is omitted.");

            InBooleans(
                "RemoveInsulation",
                "If true, the take-off is placed without insulation (input only 1 to use the same value for all). " +
                "Optional.");

            SetOptionality(new[] { 1, 3, 4, 5 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each placed take-off.");

            OutErrorMessages(
                "Error message of each take-off (empty when it was placed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElectricalComponentHelp.TryGetElementIds(this, da, 0, out List<ElementGuid> segmentIds))
            {
                return;
            }
            if (segmentIds.Count == 0)
            {
                this.AddError("The SegmentGuids input must contain at least one segment.");
                return;
            }

            if (!da.TryGetList(2, out List<double> distances))
            {
                return;
            }
            if (distances.Count != 1 &&
                distances.Count != segmentIds.Count)
            {
                this.AddError(
                    "The size of the input DistancesFromBeginning must be 1 or equal to the size of the input SegmentGuids.");
                return;
            }

            var fields = new ElectricalItemFields(this, 0, segmentIds.Count);
            var defaultsFields = new ElectricalItemFields(this, 0, segmentIds.Count);
            if (!fields.AddTexts(da, 1, "domain") ||
                !fields.AddNumbers(da, 3, "rotationAroundMainAxis") ||
                !fields.AddNumbers(da, 4, "length") ||
                !defaultsFields.AddBooleans(da, 5, "removeInsulation"))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < segmentIds.Count; i++)
            {
                var item = new JObject
                {
                    ["segmentId"] = new JObject { ["guid"] = segmentIds[i].Guid },
                    ["distanceFromBeginning"] = distances[distances.Count == 1 ? 0 : i]
                };
                fields.ApplyTo(item, i);
                var defaults = new JObject();
                defaultsFields.ApplyTo(defaults, i);
                if (defaults.Count > 0)
                {
                    item["defaults"] = defaults;
                }
                items.Add(item);
            }

            SetCadValuesWithCreatedIds<ElementGuid>(
                CommandName,
                new JObject { ["takeOffsData"] = items },
                ToAddOn,
                da,
                "elements",
                "elementId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMEPTakeOffs;

        public override Guid ComponentGuid =>
            new Guid("5f73ac8e-8e39-45a4-8881-33a91f515e13");
    }
}
