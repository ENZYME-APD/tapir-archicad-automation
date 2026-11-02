using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using Arc = TapirGrasshopperPlugin.Types.Element.Arc;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetPolygonFromUserComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetPolygonFromUser";

        public GetPolygonFromUserComponent()
            : base(
                "GetPolygonFromUser",
                "Waits for the user to input a polygon in the active Archicad window and returns its points and arcs. " +
                "Archicad waits for the input or for Escape, and every other command queues behind this one until then.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "StartPrompt",
                "Prompt for selecting the polygon start point. Optional.");

            InText(
                "Prompt",
                "Prompt shown while placing the remaining polygon nodes. Optional.");

            InText(
                "Method",
                "Polygon input method: General (free polygon, the default), Rect (axis-aligned rectangle), " +
                "RotRect (rotated rectangle) or Polyline (open polyline). Optional.");

            SetOptionality(new[] { 0, 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutCurves(
                "Polygon",
                "The polygon the user input, with its arcs (empty when cancelled).");

            OutPoints(
                "Points",
                "The nodes of the polygon.");

            OutIntegers(
                "ArcBegIndices",
                "Node index of one end point of each arc segment.");

            OutIntegers(
                "ArcEndIndices",
                "Node index of the other end point of each arc segment.");

            OutNumberList(
                "ArcAngles",
                "Angle of each arc segment; positive when the arc is on the right-hand side of the straight segment.");

            OutBoolean(
                "Cancelled",
                "True when the user cancelled the input with Escape.");
        }

        // The polygon input is modal in Archicad and blocks every other
        // command, so it is only asked for on the Refresh button, never on
        // placement or on an upstream change.
        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            if (!ManualRefreshRequested)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Press Refresh to ask for a polygon in Archicad");
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
                    out string startPrompt) &&
                !string.IsNullOrEmpty(startPrompt))
            {
                parameters["startPrompt"] = startPrompt;
            }

            if (da.TryGet(
                    1,
                    out string prompt) &&
                !string.IsNullOrEmpty(prompt))
            {
                parameters["prompt"] = prompt;
            }

            var method = "General";
            if (da.TryGet(
                    2,
                    out string methodInput) &&
                !string.IsNullOrEmpty(methodInput))
            {
                method = methodInput;
                parameters["method"] = methodInput;
            }

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
                5,
                cancelled);

            if (cancelled)
            {
                return;
            }

            var points = response["points"]?.ToObject<List<Point2D>>() ??
                         new List<Point2D>();
            var arcs = (response["arcs"]?.ToObject<List<Arc>>() ??
                        new List<Arc>())
                .OrderBy(arc => arc.BegIndex)
                .ToList();

            // The curve of a closed polygon ends where it starts.
            var curvePoints = new List<Point2D>(points);
            if (method != "Polyline" &&
                curvePoints.Count > 2 &&
                (curvePoints[0].X != curvePoints[curvePoints.Count - 1].X ||
                 curvePoints[0].Y != curvePoints[curvePoints.Count - 1].Y))
            {
                curvePoints.Add(curvePoints[0]);
            }

            da.SetDataList(
                0,
                curvePoints.Count > 1
                    ? new List<Curve>
                    {
                        Helps.Convert.ToPolyCurve(
                            curvePoints,
                            arcs,
                            0.0)
                    }
                    : new List<Curve>());

            da.SetDataList(
                1,
                points.Select(
                    point => new Point3d(
                        point.X,
                        point.Y,
                        0.0)));

            da.SetDataList(
                2,
                arcs.Select(arc => arc.BegIndex));

            da.SetDataList(
                3,
                arcs.Select(arc => arc.EndIndex));

            da.SetDataList(
                4,
                arcs.Select(arc => arc.arcAngle));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPolygonFromUser;

        public override Guid ComponentGuid =>
            new Guid("49e9be64-7c05-419f-a1f7-0337f3d810d3");
    }
}
