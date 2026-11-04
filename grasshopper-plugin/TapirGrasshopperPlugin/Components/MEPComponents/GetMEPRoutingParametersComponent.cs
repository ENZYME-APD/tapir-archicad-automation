using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPRoutingParametersComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPRoutingParameters";

        public GetMEPRoutingParametersComponent()
            : base(
                "GetMEPRoutingParameters",
                "Gets the routing parameters of placed MEP elements: of a routing element its routing node " +
                "parameters (preferred transition placement, elbow from Archicad 29, transition) and routing " +
                "segment parameters (cross section, preference table, anchor, reference line offsets, wall " +
                "thickness, flange size, connection style); of a routing node its elbow (from Archicad 29) and " +
                "transition parameters; of a routing segment its rigid segment parameters (Archicad 28 only). " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of MEP routing elements, routing nodes or routing segments.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Types",
                "Type of each element: RoutingElement, RoutingNode or RoutingSegment (null when it failed).");

            OutTexts(
                "RoutingNodeParameters",
                "The routing node default parameters of each routing element as JSON.");

            OutTexts(
                "RoutingSegmentParameters",
                "The routing segment default parameters of each routing element as JSON.");

            OutTexts(
                "ElbowParameters",
                "The elbow parameters of each routing node as JSON (Archicad 29 or newer).");

            OutTexts(
                "TransitionParameters",
                "The transition parameters of each routing node as JSON.");

            OutTexts(
                "RigidSegmentParameters",
                "The rigid segment default parameters of each routing segment as JSON (Archicad 28 only).");

            OutErrorMessages(
                "Error message of each element (empty when its parameters were got).");
        }

        private static string TextOf(
            JToken item,
            string key)
        {
            var token = item?[key];
            return token == null || token.Type == JTokenType.Null
                ? null
                : token.ToString(Formatting.Indented);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var keys = new[]
            {
                "routingNodeParameters",
                "routingSegmentParameters",
                "elbowParameters",
                "transitionParameters",
                "rigidSegmentParameters"
            };
            var types = new List<object>();
            var outputs = new List<List<string>>();
            foreach (var key in keys)
            {
                outputs.Add(new List<string>());
            }
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "routingParameters"))
            {
                var isError = JsonOutputHelp.IsError(item);
                types.Add(isError ? null : JsonOutputHelp.Scalar(item, "type"));
                for (var k = 0; k < keys.Length; k++)
                {
                    outputs[k].Add(isError ? null : TextOf(item, keys[k]));
                }
                errors.Add(isError ? JsonOutputHelp.ErrorMessage(item) : "");
            }

            da.SetDataList(0, types);
            for (var k = 0; k < keys.Length; k++)
            {
                da.SetDataList(k + 1, outputs[k]);
            }
            da.SetDataList(keys.Length + 1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPRoutingElements;

        public override Guid ComponentGuid =>
            new Guid("a50be71f-f071-4839-adbf-9e233e1b8fed");
    }
}
