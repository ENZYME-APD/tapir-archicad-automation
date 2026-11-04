using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class ModifyMEPElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyMEPElements";

        private const int ControlPointsIndex = 5;
        private const int PolylinesIndex = 6;

        public ModifyMEPElementsComponent()
            : base(
                "ModifyMEPElements",
                "Modify placed MEP elements: the anchor point, orientation and library part of terminals, accessories, " +
                "equipment, fittings and flexible segments, the control points, polyline (Piping, from Archicad 28), " +
                "and width and height of flexible segments. " +
                "Only the given fields are changed. Available from Archicad 27.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the MEP elements to modify.");

            InPoints(
                "AnchorPoints",
                "New anchor point of each element (input only 1 to use the same point for all). Optional.");

            InVectors(
                "OrientationDirections",
                "Direction vector of the new orientation of each element; give OrientationRotations too " +
                "(input only 1 to use the same direction for all). Optional.");

            InVectors(
                "OrientationRotations",
                "Rotation vector of the new orientation of each element; give OrientationDirections too " +
                "(input only 1 to use the same rotation for all). Optional.");

            InTexts(
                "LibraryPartGuids",
                "Guid of the new library part of each terminal, accessory, equipment or fitting, as listed by " +
                "GetAvailableLibraryParts (input only 1 to use the same library part for all). Optional.");

            inManager.AddPointParameter(
                "ControlPoints",
                "ControlPoints",
                "New control points of each flexible segment: one branch per element, at least 2 points " +
                "(a single branch is used for all). Optional.",
                GH_ParamAccess.tree);

            InTexts(
                "Polylines",
                "New polyline of each Piping flexible segment as JSON, the 'polyline' array GetMEPElementDetails returns: " +
                "alternating parts {\"type\": \"Line\", \"begin\", \"end\"} and {\"type\": \"Spline\", \"controlPoints\"} " +
                "that connect at their ends (input only 1 to use the same polyline for all; Archicad 28 or newer). Optional.");

            InNumbers(
                "Widths",
                "New width of each flexible segment (input only 1 to use the same width for all). Optional.");

            InNumbers(
                "Heights",
                "New height of each flexible segment (input only 1 to use the same height for all). Optional.");

            SetOptionality(new[] { 1, 2, 3, 4, ControlPointsIndex, PolylinesIndex, 7, 8 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each MEP element (empty when it was modified successfully).");
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
            if (!fields.AddPoints(da, 1, "anchorPoint") ||
                !fields.AddOrientations(da, 2, 3, "orientation") ||
                !fields.AddLibraryPartIds(da, 4, "libraryPartId") ||
                !fields.AddNumbers(da, 7, "width") ||
                !fields.AddNumbers(da, 8, "height"))
            {
                return;
            }

            if (!TryGetControlPoints(da, ids.Count, out var controlPoints) ||
                !TryGetPolylines(da, ids.Count, out var polylines))
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
                if (controlPoints.Count > 0)
                {
                    item["controlPoints"] = controlPoints[controlPoints.Count == 1 ? 0 : i].DeepClone();
                }
                if (polylines.Count > 0)
                {
                    item["polyline"] = polylines[polylines.Count == 1 ? 0 : i].DeepClone();
                }
                items.Add(item);
            }

            var parameters = new JObject { ["elements"] = items };

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

        // The control points of each element: 0 branches (unchanged), 1 (all) or one per element.
        private bool TryGetControlPoints(
            IGH_DataAccess da,
            int itemCount,
            out List<JArray> controlPoints)
        {
            controlPoints = new List<JArray>();
            if (!da.TryGetTree(ControlPointsIndex, out GH_Structure<GH_Point> tree) ||
                tree == null || tree.DataCount == 0)
            {
                return true;
            }
            if (tree.Branches.Count != 1 && tree.Branches.Count != itemCount)
            {
                this.AddError(
                    "The number of branches of the input ControlPoints must be 0, 1 or equal to the size of the input ElementGuids.");
                return false;
            }
            foreach (var branch in tree.Branches)
            {
                if (branch.Count < 2)
                {
                    this.AddError("Each branch of the input ControlPoints must contain at least 2 points.");
                    return false;
                }
                var points = new JArray();
                foreach (var ghPoint in branch)
                {
                    points.Add(ElectricalComponentHelp.Coordinate(ghPoint.Value));
                }
                controlPoints.Add(points);
            }
            return true;
        }

        // The polyline of each element, each a JSON array of line and spline parts.
        private bool TryGetPolylines(
            IGH_DataAccess da,
            int itemCount,
            out List<JArray> polylines)
        {
            polylines = new List<JArray>();
            da.TryGetList(PolylinesIndex, out List<string> texts);
            texts = texts ?? new List<string>();
            if (texts.Count > 1 && texts.Count != itemCount)
            {
                this.AddError(
                    "The size of the input Polylines must be 0, 1 or equal to the size of the input ElementGuids.");
                return false;
            }
            foreach (var text in texts)
            {
                JArray parts;
                try
                {
                    parts = JArray.Parse(text ?? "");
                }
                catch (JsonException)
                {
                    this.AddError("Each item of the input Polylines must be a JSON array of polyline parts.");
                    return false;
                }
                foreach (var token in parts)
                {
                    var part = token as JObject;
                    var type = part?["type"]?.ToString();
                    var isLine = type == "Line" && part["begin"] != null && part["end"] != null;
                    var isSpline = type == "Spline" && part["controlPoints"] is JArray;
                    if (!isLine && !isSpline)
                    {
                        this.AddError(
                            "Each part of a polyline must be {\"type\": \"Line\", \"begin\", \"end\"} or {\"type\": \"Spline\", \"controlPoints\"}.");
                        return false;
                    }
                }
                polylines.Add(parts);
            }
            return true;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMEPRoutingElements;

        public override Guid ComponentGuid =>
            new Guid("14ae9039-148d-47d4-a23e-4153a268e23b");
    }
}
