using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class CreateMEPFlexibleSegmentsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateMEPFlexibleSegments";

        private const int ControlPointsIndex = 0;
        private const int PolylinesIndex = 6;

        public CreateMEPFlexibleSegmentsComponent()
            : base(
                "CreateMEPFlexibleSegments",
                "Creates MEP flexible segments through the given control points, or along the given curves " +
                "(Piping only), with the current settings of the Flexible Segment tool of their domain, " +
                "optionally with the orientation (control points) or direction (curves) at both ends. " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddPointParameter(
                "ControlPoints",
                "ControlPoints",
                "Control points of each flexible segment from its beginning to its end " +
                "(one branch per segment, at least 2 points). Give either ControlPoints or Polylines.",
                GH_ParamAccess.tree);

            InTexts(
                "Domains",
                "MEP domain of each flexible segment: Ventilation, Piping or CableCarrier " +
                "(input only 1 to use the same domain for all).");

            InVectors(
                "StartDirections",
                "Direction vector of the orientation at the beginning of each segment; give StartRotations too " +
                "(input only 1 to use the same vector for all). Only with ControlPoints. Optional; defaults to the " +
                "direction from the first control point to the second one.");

            InVectors(
                "StartRotations",
                "Rotation vector of the orientation at the beginning of each segment; give StartDirections too " +
                "(input only 1 to use the same vector for all). Only with ControlPoints. Optional; defaults to a " +
                "horizontal vector perpendicular to the direction.");

            InVectors(
                "EndDirections",
                "Direction vector of the orientation at the end of each segment; give EndRotations too " +
                "(input only 1 to use the same vector for all). Only with ControlPoints. Optional; defaults to the " +
                "direction from the last but one control point to the last one.");

            InVectors(
                "EndRotations",
                "Rotation vector of the orientation at the end of each segment; give EndDirections too " +
                "(input only 1 to use the same vector for all). Only with ControlPoints. Optional; defaults to a " +
                "horizontal vector perpendicular to the direction.");

            inManager.AddCurveParameter(
                "Polylines",
                "Polylines",
                ("The curve of each flexible segment, instead of ControlPoints: its straight segments become " +
                "line parts and its curved segments spline parts through points on them (as many as the " +
                "curved segment has control points). Line and spline parts must alternate, so two straight " +
                "segments may not follow each other. Archicad places these flexible segments in the Piping " +
                "domain only. Optional.")
                    .WithTypeName("curveList"),
                GH_ParamAccess.list);

            InVectors(
                "PolylineStartDirections",
                "Direction vector at the beginning of each curve of Polylines, only when it begins with a curved " +
                "segment (input only 1 to use the same vector for all). Optional.");

            InVectors(
                "PolylineEndDirections",
                "Direction vector at the end of each curve of Polylines, only when it ends with a curved " +
                "segment (input only 1 to use the same vector for all). Optional.");

            SetOptionality(new[] { 0, 2, 3, 4, 5, 6, 7, 8 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each placed flexible segment.");

            OutErrorMessages(
                "Error message of each flexible segment (empty when it was placed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGetTree(
                ControlPointsIndex,
                out GH_Structure<GH_Point> controlPoints);
            var controlPointSegmentCount =
                controlPoints == null || controlPoints.DataCount == 0 ? 0 : controlPoints.Branches.Count;

            da.TryGetList(
                PolylinesIndex,
                out List<Curve> curves);
            curves = curves ?? new List<Curve>();

            if (controlPointSegmentCount > 0 && curves.Count > 0)
            {
                this.AddError("Give either the ControlPoints or the Polylines input, not both.");
                return;
            }

            var usePolylines = curves.Count > 0;
            var segmentCount = usePolylines ? curves.Count : controlPointSegmentCount;
            if (segmentCount == 0)
            {
                this.AddError("The ControlPoints or the Polylines input must contain at least one flexible segment.");
                return;
            }

            foreach (var index in usePolylines ? new[] { 2, 3, 4, 5 } : new[] { 7, 8 })
            {
                if (Params.Input[index].VolatileDataCount > 0)
                {
                    this.AddError(
                        $"The input {Params.Input[index].Name} only applies to " +
                        (usePolylines ? "ControlPoints." : "Polylines."));
                    return;
                }
            }

            if (!da.TryGetList(1, out List<string> domains))
            {
                return;
            }
            if (domains.Count != 1 &&
                domains.Count != segmentCount)
            {
                this.AddError(
                    "The size of the input Domains must be 1 or equal to the number of flexible segments " +
                    "(branches of ControlPoints or curves of Polylines).");
                return;
            }

            var fields = new ElectricalItemFields(
                this,
                usePolylines ? PolylinesIndex : ControlPointsIndex,
                segmentCount);
            if (usePolylines)
            {
                if (!fields.AddVectors(da, 7, "startDirection") ||
                    !fields.AddVectors(da, 8, "endDirection"))
                {
                    return;
                }
            }
            else if (!fields.AddOrientations(da, 2, 3, "startOrientation") ||
                     !fields.AddOrientations(da, 4, 5, "endOrientation"))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < segmentCount; i++)
            {
                var item = new JObject
                {
                    ["domain"] = domains[domains.Count == 1 ? 0 : i]
                };

                if (usePolylines)
                {
                    if (!TryGetPolylineParts(
                            curves[i],
                            out JArray parts,
                            out string error))
                    {
                        this.AddError($"Polylines item {i}: {error}.");
                        return;
                    }
                    item["polyline"] = parts;
                }
                else
                {
                    var branch = controlPoints.Branches[i];
                    if (branch.Count < 2)
                    {
                        this.AddError("Each flexible segment (branch) must contain at least 2 control points.");
                        return;
                    }

                    var points = new JArray();
                    foreach (var ghPoint in branch)
                    {
                        points.Add(ElectricalComponentHelp.Coordinate(ghPoint.Value));
                    }
                    item["controlPoints"] = points;
                }

                fields.ApplyTo(item, i);
                items.Add(item);
            }

            SetCadValuesWithCreatedIds<ElementGuid>(
                CommandName,
                new JObject { ["flexibleSegmentsData"] = items },
                ToAddOn,
                da,
                "elements",
                "elementId");
        }

        // The 'polyline' parts (MEPFlexibleSegmentPolyline) of a curve: a
        // "Line" part for each straight segment, a "Spline" part through
        // points on each run of curved segments (as many points as a curved
        // segment has control points, at least 3; following curved segments
        // share their end points and make one spline part).
        private static bool TryGetPolylineParts(
            Curve curve,
            out JArray parts,
            out string error)
        {
            parts = new JArray();
            error = null;

            if (curve == null)
            {
                error = "the curve is null";
                return false;
            }

            var segments = curve is PolyCurve polyCurve
                ? polyCurve.DuplicateSegments()
                : curve.TryGetPolyline(out Polyline polyline)
                    ? polyline.GetSegments().Select(s => (Curve) new LineCurve(s)).ToArray()
                    : new[] { curve };

            if (segments == null || segments.Length == 0)
            {
                error = "the curve has no segments";
                return false;
            }

            List<Point3d> splinePoints = null;
            foreach (var segment in segments)
            {
                if (segment.IsLinear())
                {
                    if (splinePoints != null)
                    {
                        parts.Add(SplinePart(splinePoints));
                        splinePoints = null;
                    }

                    parts.Add(new JObject
                    {
                        ["type"] = "Line",
                        ["begin"] = ElectricalComponentHelp.Coordinate(segment.PointAtStart),
                        ["end"] = ElectricalComponentHelp.Coordinate(segment.PointAtEnd)
                    });
                    continue;
                }

                if (segment.IsClosed)
                {
                    error = "a closed curved segment cannot be a spline part";
                    return false;
                }

                var nurbs = segment.ToNurbsCurve();
                var divisionCount = Math.Max(2, (nurbs?.Points.Count ?? 3) - 1);
                var parameters = segment.DivideByCount(divisionCount, true);
                if (parameters == null || parameters.Length < 2)
                {
                    error = "a curved segment could not be divided into points";
                    return false;
                }

                // The first point of a following curved segment is the last
                // point of the one before it.
                var first = 0;
                if (splinePoints == null)
                {
                    splinePoints = new List<Point3d>();
                }
                else
                {
                    first = 1;
                }

                for (var j = first; j < parameters.Length; j++)
                {
                    splinePoints.Add(segment.PointAt(parameters[j]));
                }
            }

            if (splinePoints != null)
            {
                parts.Add(SplinePart(splinePoints));
            }

            return true;
        }

        private static JObject SplinePart(
            List<Point3d> points)
        {
            var controlPoints = new JArray();
            foreach (var point in points)
            {
                controlPoints.Add(ElectricalComponentHelp.Coordinate(point));
            }

            return new JObject
            {
                ["type"] = "Spline",
                ["controlPoints"] = controlPoints
            };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMEPFlexibleSegments;

        public override Guid ComponentGuid =>
            new Guid("06756219-7e17-4c4f-b13e-7a5ff5e6d29c");
    }
}
