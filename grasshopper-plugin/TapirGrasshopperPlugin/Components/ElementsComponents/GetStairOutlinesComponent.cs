using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // The plan geometry of stairs as curves, read with GetDetailsOfElements:
    // the baseline, the left and right boundaries, the walking line and the
    // closed outline made of the boundaries (#2936).
    public class GetStairOutlinesComponent : ElementsStructuredGetterComponent
    {
        // Two points closer than this (in meters) are joined without a line.
        private const double JoinTolerance = 1e-6;

        public override string CommandName => "GetDetailsOfElements";

        public GetStairOutlinesComponent()
            : base(
                "StairOutlines",
                "Gets the plan geometry of stairs as curves, with their arcs, at the elevation of each stair's " +
                "base: the baseline, the left and right boundaries, the walking line, and the closed outline " +
                "in plan (the left boundary, then the right boundary reversed, joined at both ends). The " +
                "outputs are aligned with the input list of elements; a curve Archicad has not calculated for " +
                "a stair is null.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "detailsOfElements";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Stair elements.");
        }

        protected override void AddOutputs()
        {
            OutCurves(
                "BaseLines",
                "The baseline of each stair, with its arcs, from the bottom of the stair to its top.");

            OutCurves(
                "LeftBoundaries",
                "The left boundary line of each stair, seen along the baseline.");

            OutCurves(
                "RightBoundaries",
                "The right boundary line of each stair, seen along the baseline.");

            OutCurves(
                "WalkingLines",
                "The walking line of each stair, along which Archicad lays out the goings.");

            OutCurves(
                "Outlines",
                "The closed outline of each stair in plan: the left boundary, then the right boundary " +
                "reversed, joined by straight lines at the start and at the end of the stair.");

            OutErrorMessages(
                "Error message of each element (empty when successful), e.g. when it is not a stair.");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var baseLines = new List<Curve>();
            var leftBoundaries = new List<Curve>();
            var rightBoundaries = new List<Curve>();
            var walkingLines = new List<Curve>();
            var outlines = new List<Curve>();
            var errors = new List<string>();

            foreach (var item in items)
            {
                var details = item?["details"];
                string error = null;
                if (JsonOutputHelp.IsError(item))
                {
                    error = JsonOutputHelp.ErrorMessage(item);
                }
                else if (item?.Value<string>("type") != "Stair" || details == null)
                {
                    error = "The element is not a stair.";
                }

                if (error != null)
                {
                    baseLines.Add(null);
                    leftBoundaries.Add(null);
                    rightBoundaries.Add(null);
                    walkingLines.Add(null);
                    outlines.Add(null);
                    errors.Add(error);
                    continue;
                }

                var z = details.Value<double?>("zCoordinate") ?? 0.0;
                var left = ToOpenCurve(details["leftBoundary"], z);
                var right = ToOpenCurve(details["rightBoundary"], z);

                var baseLine = PolygonHelp.ToCurve(
                    details["baseLinePoints"],
                    details["baseLineArcs"],
                    false,
                    z);

                // baseLinePoints run from the top down on an Inverse stair;
                // the boundaries and the walking line always run bottom-up.
                if (baseLine != null &&
                    details.Value<string>("inputDirection") == "Inverse")
                {
                    baseLine.Reverse();
                }

                baseLines.Add(baseLine);
                leftBoundaries.Add(left);
                rightBoundaries.Add(right);
                walkingLines.Add(ToOpenCurve(details["walkingLine"], z));
                outlines.Add(ToOutline(left, right));
                errors.Add("");
            }

            da.SetDataList(0, baseLines);
            da.SetDataList(1, leftBoundaries);
            da.SetDataList(2, rightBoundaries);
            da.SetDataList(3, walkingLines);
            da.SetDataList(4, outlines);
            da.SetDataList(5, errors);
        }

        // A stair polyline ({"points", "arcs"}: StairBoundary, StairPolyline)
        // as a curve; null when it is not given.
        private static Curve ToOpenCurve(
            JToken polyline,
            double z)
        {
            if (polyline == null)
            {
                return null;
            }

            return PolygonHelp.ToCurve(
                polyline["points"],
                polyline["arcs"],
                false,
                z);
        }

        // The closed outline of a stair: the left boundary, a line to the end
        // of the right boundary, the right boundary reversed and a line back to
        // the start of the left boundary. A line is left out where the two
        // boundaries already meet.
        private static Curve ToOutline(
            Curve left,
            Curve right)
        {
            if (left == null || right == null)
            {
                return null;
            }

            var reversed = right.DuplicateCurve();
            reversed.Reverse();

            var outline = new PolyCurve();
            outline.Append(left.DuplicateCurve());
            AppendLine(outline, left.PointAtEnd, reversed.PointAtStart);
            outline.Append(reversed);
            AppendLine(outline, reversed.PointAtEnd, left.PointAtStart);
            return outline;
        }

        private static void AppendLine(
            PolyCurve curve,
            Point3d from,
            Point3d to)
        {
            if (from.DistanceTo(to) > JoinTolerance)
            {
                curve.Append(new Line(from, to));
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SolveStairBaseline;

        public override Guid ComponentGuid =>
            new Guid("6d0f3b52-8c4e-4a71-9f2d-1b7e5a3c9e48");
    }
}
