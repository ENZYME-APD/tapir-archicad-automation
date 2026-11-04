using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class PlaceMEPTakeOffsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PlaceMEPTakeOffs";

        public PlaceMEPTakeOffsComponent()
            : base(
                "PlaceMEPTakeOffs",
                "Places MEP take-offs on the given routing segments, at a distance from the beginning of the " +
                "segment and optionally rotated around its axis, with the current settings of the Take-Off tool " +
                "of their domain, optionally with their length. Available from Archicad 28.",
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
                "MEP domain of each take-off: Ventilation, Piping or CableCarrier " +
                "(input only 1 to use the same domain for all).");

            InNumbers(
                "DistancesFromBeginning",
                "Distance of each take-off from the beginning of its segment, not negative " +
                "(input only 1 to use the same distance for all).");

            InNumbers(
                "RotationsAroundMainAxis",
                "Rotation of each take-off around the axis of its segment, in radians " +
                "(input only 1 to use the same rotation for all). Optional; 0 when it is omitted.");

            InNumbers(
                "Lengths",
                "Length of each take-off, set after placing it (input only 1 to use the same length for all). " +
                "Optional; the length of the Take-Off tool when it is omitted.");

            SetOptionality(new[] { 3, 4 });
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

            if (!da.TryGetList(1, out List<string> domains) ||
                !da.TryGetList(2, out List<double> distances))
            {
                return;
            }
            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("Domains", domains.Count),
                         ("DistancesFromBeginning", distances.Count)
                     })
            {
                if (pair.Count != 1 &&
                    pair.Count != segmentIds.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 1 or equal to the size of the input SegmentGuids.");
                    return;
                }
            }

            var fields = new ElectricalItemFields(this, 0, segmentIds.Count);
            if (!fields.AddNumbers(da, 3, "rotationAroundMainAxis") ||
                !fields.AddNumbers(da, 4, "length"))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < segmentIds.Count; i++)
            {
                var item = new JObject
                {
                    ["domain"] = domains[domains.Count == 1 ? 0 : i],
                    ["segmentId"] = new JObject { ["guid"] = segmentIds[i].Guid },
                    ["distanceFromBeginning"] = distances[distances.Count == 1 ? 0 : i]
                };
                fields.ApplyTo(item, i);
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
            Properties.Resources.CreateMEPElements;

        public override Guid ComponentGuid =>
            new Guid("5f73ac8e-8e39-45a4-8881-33a91f515e13");
    }
}
