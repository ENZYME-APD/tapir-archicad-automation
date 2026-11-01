using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPElementDetailsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPElementDetails";

        public GetMEPElementDetailsComponent()
            : base(
                "GetMEPElementDetails",
                "Retrieves the type-specific details of the given MEP elements: insulation thickness and routing segment of " +
                "rigid segments, control points of flexible segments, radius factor, insulation thickness, preference table " +
                "and routing node of elbows (from Archicad 29), angle, length and insulation thickness of branches, and " +
                "offsets, wider and narrower ports, insulation thickness, routing node and (from Archicad 29) preference " +
                "table of transitions. A value that does not belong to the type of an element is empty. " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the MEP elements.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Types",
                "MEP element type of each element (RigidSegment, FlexibleSegment, Elbow, Branch, Transition, ...).");

            OutTexts(
                "Domains",
                "MEP domain of each element (Ventilation, Piping or CableCarrier; empty if it has none).");

            OutNumberList(
                "InsulationThicknesses",
                "Insulation thickness of each rigid segment, elbow, branch and transition.");

            OutGenerics(
                "RoutingSegmentGuids",
                "Routing segment each rigid segment belongs to (see GetMEPRoutingElements).");

            OutGenerics(
                "RoutingNodeGuids",
                "Routing node each elbow and transition belongs to (see GetMEPRoutingElements).");

            OutPointTree(
                "ControlPoints",
                "Control points of each flexible segment (one branch per element).");

            OutNumberList(
                "RadiusFactors",
                "Radius factor of each elbow (Archicad 29 or newer).");

            OutTexts(
                "PreferenceTableGuids",
                "Guid of the preference table of each elbow and transition (see GetMEPPreferenceTables; Archicad 29 or newer).");

            OutNumberList(
                "BranchAngles",
                "Angle of each branch.");

            OutNumberList(
                "BranchLengths",
                "Length of each branch.");

            OutNumberList(
                "OffsetsY",
                "Y offset of each transition.");

            OutNumberList(
                "OffsetsZ",
                "Z offset of each transition.");

            OutTexts(
                "WiderPortIds",
                "Port at the wider end of each transition (see GetMEPPorts).");

            OutTexts(
                "NarrowerPortIds",
                "Port at the narrower end of each transition (see GetMEPPorts).");

            OutBooleans(
                "IsControlledByPreference",
                "Whether each transition is controlled by its preference table (Archicad 29 or newer).");

            OutErrorMessages(
                "Error message of each element (empty when its details were read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElectricalComponentHelp.TryGetElementIds(this, da, 0, out var ids) ||
                ids.Count == 0)
            {
                return;
            }

            var parameters = new JObject
            {
                ["elements"] = ElectricalComponentHelp.ElementsArray(ids)
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            // The scalar outputs, in output order, by their key.
            var scalarKeys = new[]
            {
                "type",
                "domain",
                "insulationThickness"
            };
            var laterScalarKeys = new[]
            {
                "factorRadius",
                "preferenceTableId",
                "branchAngle",
                "branchLength",
                "offsetY",
                "offsetZ",
                "widerPortId",
                "narrowerPortId",
                "isControlledByPreference"
            };

            var scalars = new List<object>[scalarKeys.Length];
            for (var k = 0; k < scalars.Length; k++)
            {
                scalars[k] = new List<object>();
            }
            var laterScalars = new List<object>[laterScalarKeys.Length];
            for (var k = 0; k < laterScalars.Length; k++)
            {
                laterScalars[k] = new List<object>();
            }
            var routingSegments = new List<object>();
            var routingNodes = new List<object>();
            var controlPoints = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(response, "elementDetails");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                controlPoints.EnsurePath(path);

                var isError = JsonOutputHelp.IsError(item);
                errors.Add(isError ? JsonOutputHelp.ErrorMessage(item) : "");
                var details = isError ? null : item;

                for (var k = 0; k < scalarKeys.Length; k++)
                {
                    scalars[k].Add(JsonOutputHelp.Scalar(details, scalarKeys[k]));
                }
                for (var k = 0; k < laterScalarKeys.Length; k++)
                {
                    laterScalars[k].Add(JsonOutputHelp.Scalar(details, laterScalarKeys[k]));
                }
                routingSegments.Add(ElectricalComponentHelp.ElementIdOf(details?["routingSegmentId"]));
                routingNodes.Add(ElectricalComponentHelp.ElementIdOf(details?["routingNodeId"]));

                if (details?["controlPoints"] is JArray points)
                {
                    foreach (var point in points)
                    {
                        controlPoints.Add(ElectricalComponentHelp.PointOf(point), path);
                    }
                }
            }

            da.SetDataList(0, scalars[0]);
            da.SetDataList(1, scalars[1]);
            da.SetDataList(2, scalars[2]);
            da.SetDataList(3, routingSegments);
            da.SetDataList(4, routingNodes);
            da.SetDataTree(5, controlPoints);
            for (var k = 0; k < laterScalars.Length; k++)
            {
                da.SetDataList(6 + k, laterScalars[k]);
            }
            da.SetDataList(6 + laterScalars.Length, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPElementDetails;

        public override Guid ComponentGuid =>
            new Guid("b2832df6-0ada-4935-a3ca-4877750b9204");
    }
}
