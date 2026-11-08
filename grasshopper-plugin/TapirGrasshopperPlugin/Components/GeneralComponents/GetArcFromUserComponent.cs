using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetArcFromUserComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetArcFromUser";

        public GetArcFromUserComponent()
            : base(
                "GetArcFromUser",
                "Asks the user to draw an arc in the active Archicad window - its center, its start point (which sets " +
                "the radius) and its end point - and returns it with its center, radius and angles. " +
                "Archicad waits for the clicks or for Escape, and every other command queues behind this one until then.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "CenterPrompt",
                "Prompt for clicking the center of the arc. Single-byte text. Optional.");

            InText(
                "StartPrompt",
                "Prompt for clicking the start point of the arc, which also sets its radius. Optional.");

            InText(
                "EndPrompt",
                "Prompt for clicking the end point of the arc. The direction the cursor goes around the center " +
                "decides whether the arc is clockwise. Optional.");

            inManager.AddBooleanParameter(
                "ShowRubberArcFeedback",
                "ShowRubberArcFeedback",
                "If true, draws the whole circle of the arc while the start and end points are chosen (2D windows only). Optional; default: true."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(new[] { 0, 1, 2, 3 });

            // The snap filter applies to the center and the start point (the end
            // point snaps as usual), the constraint to every click.
            UserInputOptions.AddInputs(this);
        }

        protected override void AddOutputs()
        {
            OutCurves(
                "Arc",
                "The arc the user drew, in the plane of its center (empty when cancelled).");

            OutPoints(
                "Center",
                "The center of the arc.");

            OutNumberList(
                "Radius",
                "The radius of the arc.");

            OutNumberList(
                "StartAngle",
                "The direction of the start point from the center, in radians from the X axis, counterclockwise.");

            OutNumberList(
                "EndAngle",
                "The direction of the end point from the center, in radians from the X axis, counterclockwise.");

            OutNumberList(
                "ArcAngle",
                "The angle the arc turns through, in radians: positive for a counterclockwise arc, negative for a clockwise one.");

            OutBoolean(
                "Clockwise",
                "True if the arc was drawn clockwise.");

            OutPoints(
                "StartPoint",
                "The start point of the arc.");

            OutPoints(
                "EndPoint",
                "The end point of the arc.");

            OutBoolean(
                "Cancelled",
                "True when the user cancelled the input with Escape.");
        }

        // The arc input is modal in Archicad and blocks every other command,
        // so it is only asked for on the Refresh button, never on placement or
        // on an upstream change.
        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            if (!ManualRefreshRequested)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Press Refresh to ask for an arc in Archicad");
                return;
            }

            Solve(da);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGet(
                    0,
                    out string centerPrompt) &&
                !string.IsNullOrEmpty(centerPrompt))
            {
                parameters["centerPrompt"] = centerPrompt;
            }

            if (da.TryGet(
                    1,
                    out string startPrompt) &&
                !string.IsNullOrEmpty(startPrompt))
            {
                parameters["startPrompt"] = startPrompt;
            }

            if (da.TryGet(
                    2,
                    out string endPrompt) &&
                !string.IsNullOrEmpty(endPrompt))
            {
                parameters["endPrompt"] = endPrompt;
            }

            if (da.TryGet(
                    3,
                    out bool showRubberArcFeedback))
            {
                parameters["showRubberArcFeedback"] = showRubberArcFeedback;
            }

            UserInputOptions.AddParameters(
                da,
                4,
                parameters);

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var cancelled = (bool?)response["cancelled"] == true;
            da.SetData(
                9,
                cancelled);

            if (cancelled)
            {
                return;
            }

            var center = JsonOutputHelp.Point(response["center"]);
            var startPoint = JsonOutputHelp.Point(response["startPoint"]);
            var endPoint = JsonOutputHelp.Point(response["endPoint"]);
            var radius = (double?)response["radius"];
            var startAngle = (double?)response["startAngle"];
            var endAngle = (double?)response["endAngle"];
            var arcAngle = (double?)response["arcAngle"];

            var curves = new List<Curve>();
            if (center is Point3d c &&
                radius is double r &&
                startAngle is double start &&
                arcAngle is double sweep)
            {
                if (Math.Abs(sweep) >= 2.0 * Math.PI - 1e-9)
                {
                    // A full turn ends where it starts: a circle.
                    curves.Add(
                        new ArcCurve(
                            new Circle(
                                c,
                                r)));
                }
                else
                {
                    // Through its start, middle and end point, so a clockwise
                    // arc keeps its direction.
                    var middleAngle = start + sweep / 2.0;
                    var endAngleOfSweep = start + sweep;
                    var arc = new Arc(
                        new Point3d(
                            c.X + r * Math.Cos(start),
                            c.Y + r * Math.Sin(start),
                            c.Z),
                        new Point3d(
                            c.X + r * Math.Cos(middleAngle),
                            c.Y + r * Math.Sin(middleAngle),
                            c.Z),
                        new Point3d(
                            c.X + r * Math.Cos(endAngleOfSweep),
                            c.Y + r * Math.Sin(endAngleOfSweep),
                            c.Z));
                    if (arc.IsValid)
                    {
                        curves.Add(new ArcCurve(arc));
                    }
                }
            }

            da.SetDataList(
                0,
                curves);

            da.SetDataList(
                1,
                center == null
                    ? new List<object>()
                    : new List<object> { center });

            da.SetDataList(
                2,
                ToList(radius));

            da.SetDataList(
                3,
                ToList(startAngle));

            da.SetDataList(
                4,
                ToList(endAngle));

            da.SetDataList(
                5,
                ToList(arcAngle));

            da.SetData(
                6,
                (bool?)response["clockwise"] == true);

            da.SetDataList(
                7,
                startPoint == null
                    ? new List<object>()
                    : new List<object> { startPoint });

            da.SetDataList(
                8,
                endPoint == null
                    ? new List<object>()
                    : new List<object> { endPoint });
        }

        private static List<double> ToList(
            double? value)
        {
            return value is double v
                ? new List<double> { v }
                : new List<double>();
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateArcs;

        public override Guid ComponentGuid =>
            new Guid("8c3849ca-f481-49ce-8384-f6ab8b776300");
    }
}
