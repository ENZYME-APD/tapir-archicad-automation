using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class SetMEPRoutingParametersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetMEPRoutingParameters";

        public SetMEPRoutingParametersComponent()
            : base(
                "SetMEPRoutingParameters",
                "Sets the routing parameters of placed MEP elements: the routing segment parameters (cross section " +
                "and preference table) and, from Archicad 29, the routing node parameters (elbow radius factor and " +
                "preference table, transition preference table) of routing elements, and the elbow and transition " +
                "parameters of routing nodes (from Archicad 29). Only the given inputs are changed, in one undo " +
                "step with one execution result per element. Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the MEP routing elements, or of the routing nodes when ForRoutingNodes is true.");

            InTexts(
                "CrossSectionShapes",
                "New cross section shape of the segments of each routing element: Rectangular, Circular, Oval or " +
                "UShape (input only 1 to use the same shape for all). Optional; routing elements only.");

            InNumbers(
                "CrossSectionWidths",
                "New cross section width of the segments of each routing element " +
                "(input only 1 to use the same width for all). Optional; routing elements only.");

            InNumbers(
                "CrossSectionHeights",
                "New cross section height of the segments of each routing element " +
                "(input only 1 to use the same height for all). Optional; routing elements only.");

            InIntegers(
                "CrossSectionReferenceIds",
                "New cross section reference id of the segment preference table of each routing element, " +
                "resolved against PreferenceTableGuids if given, otherwise against the table already set " +
                "(input only 1 to use the same id for all). Optional; routing elements only.");

            InTexts(
                "PreferenceTableGuids",
                "New segment preference table of each routing element, as returned by GetMEPPreferenceTables " +
                "(input only 1 to use the same table for all). Optional; routing elements only.");

            InNumbers(
                "ElbowRadiusFactors",
                "New radius factor of the elbows of each element (input only 1 to use the same factor for all). " +
                "Optional; Archicad 29 or newer.");

            InTexts(
                "ElbowPreferenceTableGuids",
                "New elbow preference table of each element, as returned by GetMEPPreferenceTables " +
                "(input only 1 to use the same table for all). Optional; Archicad 29 or newer.");

            InTexts(
                "TransitionPreferenceTableGuids",
                "New transition preference table of each element, as returned by GetMEPPreferenceTables " +
                "(input only 1 to use the same table for all). Optional; Archicad 29 or newer.");

            InBoolean(
                "ForRoutingNodes",
                "If true, ElementGuids are routing nodes: the elbow and transition inputs set their elbow and " +
                "transition parameters, and the cross section and segment preference table inputs must be empty. " +
                "If false, they are routing elements: the elbow and transition inputs set their routing node " +
                "default parameters.",
                false);

            SetOptionality(new[] { 1, 2, 3, 4, 5, 6, 7, 8, 9 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when its parameters were set).");
        }

        // Reads an optional input of guid texts with 0, 1 or count items.
        private bool TryGetGuids(
            IGH_DataAccess da,
            int index,
            int count,
            out List<string> guids)
        {
            if (!MEPComponentHelp.TryGetGuidTexts(this, da, index, out guids))
            {
                return false;
            }
            return CheckCount(index, guids.Count, count);
        }

        private bool CheckCount(
            int index,
            int inputCount,
            int count)
        {
            if (inputCount > 1 &&
                inputCount != count)
            {
                this.AddError(
                    $"The size of the input {Params.Input[index].Name} must be 0, 1 or equal to the size of the input ElementGuids.");
                return false;
            }
            return true;
        }

        private static T ItemFor<T>(
            List<T> items,
            int index)
        {
            return items[items.Count == 1 ? 0 : index];
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElectricalComponentHelp.TryGetElementIds(this, da, 0, out List<ElementGuid> elementIds))
            {
                return;
            }
            var count = elementIds.Count;

            var segmentFields = new ElectricalItemFields(this, 0, count);
            if (!segmentFields.AddTexts(da, 1, "crossSectionShape") ||
                !segmentFields.AddNumbers(da, 2, "crossSectionWidth") ||
                !segmentFields.AddNumbers(da, 3, "crossSectionHeight") ||
                !segmentFields.AddIntegers(da, 4, "crossSectionReferenceId") ||
                !TryGetGuids(da, 5, count, out List<string> segmentTableGuids))
            {
                return;
            }

            da.TryGetList(6, out List<double> elbowRadiusFactors);
            elbowRadiusFactors = elbowRadiusFactors ?? new List<double>();
            if (!CheckCount(6, elbowRadiusFactors.Count, count) ||
                !TryGetGuids(da, 7, count, out List<string> elbowTableGuids) ||
                !TryGetGuids(da, 8, count, out List<string> transitionTableGuids))
            {
                return;
            }

            bool forRoutingNodes = false;
            da.GetData(9, ref forRoutingNodes);

            var items = new JArray();
            for (var i = 0; i < count; i++)
            {
                var segment = new JObject();
                segmentFields.ApplyTo(segment, i);
                if (segmentTableGuids.Count > 0)
                {
                    segment["preferenceTableId"] = ItemFor(segmentTableGuids, i);
                }

                var elbow = new JObject();
                if (elbowRadiusFactors.Count > 0)
                {
                    elbow["factorRadius"] = ItemFor(elbowRadiusFactors, i);
                }
                if (elbowTableGuids.Count > 0)
                {
                    elbow["preferenceTableId"] = ItemFor(elbowTableGuids, i);
                }

                JObject transition = null;
                if (transitionTableGuids.Count > 0)
                {
                    transition = new JObject { ["preferenceTableId"] = ItemFor(transitionTableGuids, i) };
                }

                var item = new JObject { ["elementId"] = new JObject { ["guid"] = elementIds[i].Guid } };
                if (forRoutingNodes)
                {
                    if (segment.Count > 0)
                    {
                        this.AddError(
                            "The cross section and segment preference table inputs are for routing elements; " +
                            "leave them empty when ForRoutingNodes is true.");
                        return;
                    }
                    if (elbow.Count > 0)
                    {
                        item["elbowParameters"] = elbow;
                    }
                    if (transition != null)
                    {
                        item["transitionParameters"] = transition;
                    }
                }
                else
                {
                    if (segment.Count > 0)
                    {
                        item["routingSegmentParameters"] = segment;
                    }
                    var routingNode = new JObject();
                    if (elbow.Count > 0)
                    {
                        routingNode["elbow"] = elbow;
                    }
                    if (transition != null)
                    {
                        routingNode["transition"] = transition;
                    }
                    if (routingNode.Count > 0)
                    {
                        item["routingNodeParameters"] = routingNode;
                    }
                }
                items.Add(item);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elements"] = items },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMEPRoutingElements;

        public override Guid ComponentGuid =>
            new Guid("c82e6bcd-59ba-4ad9-a130-5028aa8b69a5");
    }
}
