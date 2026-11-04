using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Types.Element;
using Arc = TapirGrasshopperPlugin.Types.Element.Arc;

namespace TapirGrasshopperPlugin.Helps
{
    // Converts between Rhino curves and the 2D polygons and polylines of the
    // Add-On: a list of coordinates ({"x", "y"}) plus the arcs ({"begIndex",
    // "endIndex", "arcAngle"}) of its arc segments. A closed polygon does not
    // repeat its first coordinate; its closing segment runs from the last
    // coordinate back to index 0.
    public static class PolygonHelp
    {
        // Line and arc segments are kept as they are, anything else is
        // approximated with a polyline. Only X and Y are used.
        public static bool TryToCoordinates(
            Curve curve,
            bool closed,
            out JArray coordinates,
            out JArray arcs,
            out string error)
        {
            coordinates = new JArray();
            arcs = new JArray();
            error = null;

            if (curve == null)
            {
                error = "a curve is null";
                return false;
            }

            if (closed && !curve.IsClosed)
            {
                error = "every polygon curve has to be closed";
                return false;
            }

            var segments = curve is PolyCurve polyCurve
                ? polyCurve.DuplicateSegments()
                : curve.TryGetPolyline(out Polyline polyline)
                    ? polyline.GetSegments().Select(s => (Curve) new LineCurve(s)).ToArray()
                    : new[] { curve };

            if (segments == null || segments.Length == 0)
            {
                error = "a curve has no segments";
                return false;
            }

            foreach (var segment in segments)
            {
                var nodeIndex = coordinates.Count;
                AddCoordinate(coordinates, segment.PointAtStart);

                if (segment.IsLinear())
                {
                    continue;
                }

                if (!segment.IsClosed &&
                    segment.TryGetArc(out Rhino.Geometry.Arc arc))
                {
                    // arcAngle is positive when the arc bulges to the right of
                    // the chord; a Rhino arc whose plane normal points up turns
                    // counter clockwise, so it bulges to the left.
                    var angle = arc.Angle;
                    if (arc.Plane.Normal.Z > 0.0)
                    {
                        angle = -angle;
                    }

                    arcs.Add(new JObject
                    {
                        ["begIndex"] = nodeIndex,
                        ["endIndex"] = nodeIndex + 1,
                        ["arcAngle"] = angle
                    });
                    continue;
                }

                var tessellated = segment.ToPolyline(0, 0, 0.05, 0.0, 0.0, 0.01, 0.0, 0.0, true)?.ToPolyline();
                if (tessellated == null)
                {
                    error = "a curve segment could not be approximated with a polyline";
                    return false;
                }

                // The segment's start is already in and its end is the next
                // segment's start, so only the points in between are added.
                for (var i = 1; i < tessellated.Count - 1; i++)
                {
                    AddCoordinate(coordinates, tessellated[i]);
                }
            }

            if (closed)
            {
                if (coordinates.Count < 3)
                {
                    error = "a polygon needs at least three points";
                    return false;
                }

                foreach (var arcToken in arcs)
                {
                    if ((int) arcToken["endIndex"] >= coordinates.Count)
                    {
                        arcToken["endIndex"] = 0;
                    }
                }
            }
            else
            {
                AddCoordinate(coordinates, segments[segments.Length - 1].PointAtEnd);
            }

            return true;
        }

        // The curve of an Add-On polygon or polyline at the given Z; null when
        // it has fewer than two coordinates.
        public static Curve ToCurve(
            JToken coordinates,
            JToken arcs,
            bool closed,
            double z = 0.0)
        {
            if (!(coordinates is JArray coordinateArray) ||
                coordinateArray.Count < 2)
            {
                return null;
            }

            var points = coordinateArray
                .Select(c => new Point2D
                {
                    X = c.Value<double?>("x") ?? 0.0,
                    Y = c.Value<double?>("y") ?? 0.0
                })
                .ToList();
            if (closed)
            {
                points.Add(points[0]);
            }

            var arcList = (arcs as JArray)?
                .Select(a => a.ToObject<Arc>())
                .OrderBy(a => a.BegIndex)
                .ToList() ?? new List<Arc>();

            var curve = Convert.ToPolyCurve(
                points,
                arcList,
                z);
            return curve.SegmentCount > 0 ? curve : null;
        }

        // The points of a list of coordinates, at the given Z.
        public static List<Point3d> ToPoints(
            JToken coordinates,
            double z = 0.0)
        {
            return (coordinates as JArray)?
                .Select(c => new Point3d(
                    c.Value<double?>("x") ?? 0.0,
                    c.Value<double?>("y") ?? 0.0,
                    c.Value<double?>("z") ?? z))
                .ToList() ?? new List<Point3d>();
        }

        private static void AddCoordinate(
            JArray coordinates,
            Point3d point)
        {
            coordinates.Add(new JObject
            {
                ["x"] = point.X,
                ["y"] = point.Y
            });
        }
    }
}
