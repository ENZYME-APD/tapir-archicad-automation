using Grasshopper;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Helps
{
    // Converts the coordinate arrays of the element commands' responses into
    // Rhino geometry.
    public static class ElementGeometryJsonHelp
    {
        // A 2D coordinate object to a point of the XY plane, or of the given
        // plane (x along its XAxis, y along its YAxis).
        public static Point3d Point2D(
            JToken coordinate,
            Plane? plane = null)
        {
            var x = coordinate?.Value<double?>("x") ?? 0.0;
            var y = coordinate?.Value<double?>("y") ?? 0.0;
            return plane.HasValue
                ? plane.Value.PointAt(
                    x,
                    y)
                : new Point3d(
                    x,
                    y,
                    0.0);
        }

        public static Point3d Point3D(
            JToken coordinate)
        {
            return new Point3d(
                coordinate?.Value<double?>("x") ?? 0.0,
                coordinate?.Value<double?>("y") ?? 0.0,
                coordinate?.Value<double?>("z") ?? 0.0);
        }

        // A polyline through the given points; closed when asked, by adding
        // the first point again unless it is already the last one. Null when
        // there are too few points.
        public static PolylineCurve Polyline(
            IEnumerable<Point3d> points,
            bool closed)
        {
            var polyline = new Polyline(points);
            if (closed &&
                polyline.Count > 2 &&
                !polyline.First.EpsilonEquals(
                    polyline.Last,
                    RhinoMath.ZeroTolerance))
            {
                polyline.Add(polyline.First);
            }
            return polyline.Count < 2 ? null : new PolylineCurve(polyline);
        }

        public static PolylineCurve Polyline2D(
            JToken coordinates,
            bool closed,
            Plane? plane = null)
        {
            if (!(coordinates is JArray array))
            {
                return null;
            }
            return Polyline(
                array.Select(c => Point2D(
                    c,
                    plane)),
                closed);
        }

        public static PolylineCurve Polyline3D(
            JToken coordinates,
            bool closed)
        {
            if (!(coordinates is JArray array))
            {
                return null;
            }
            return Polyline(
                array.Select(Point3D),
                closed);
        }
    }

    // Collects the 2D drawing primitives of GetDrawingPrimitivesOfElements and
    // GetStoryControlPrimitives into data trees, one item per primitive.
    public class DrawingPrimitivesOutput
    {
        public static string OutputsDescription =>
            "Types are Point, Line, Arc, Text, PLine, Tri, Poly, Pict and the Ctrl_* control items that " +
            "mark the beginning and the end of sub-parts such as hatches and texts.";

        public readonly DataTree<object> Types = new DataTree<object>();
        public readonly DataTree<object> Geometries = new DataTree<object>();
        public readonly DataTree<object> Pens = new DataTree<object>();
        public readonly DataTree<object> Layers = new DataTree<object>();
        public readonly DataTree<object> DrawIndices = new DataTree<object>();
        public readonly DataTree<object> RenovationStatuses =
            new DataTree<object>();
        public readonly DataTree<object> Texts = new DataTree<object>();
        public readonly DataTree<object> Primitives = new DataTree<object>();

        public void EnsurePath(
            GH_Path path)
        {
            Types.EnsurePath(path);
            Geometries.EnsurePath(path);
            Pens.EnsurePath(path);
            Layers.EnsurePath(path);
            DrawIndices.EnsurePath(path);
            RenovationStatuses.EnsurePath(path);
            Texts.EnsurePath(path);
            Primitives.EnsurePath(path);
        }

        public void AddPrimitives(
            JToken primitives,
            GH_Path path)
        {
            EnsurePath(path);
            if (!(primitives is JArray array))
            {
                return;
            }

            foreach (var primitive in array)
            {
                Types.Add(
                    JsonOutputHelp.Scalar(
                        primitive,
                        "type"),
                    path);
                Geometries.Add(
                    GeometryOf(primitive),
                    path);
                Pens.Add(
                    JsonOutputHelp.Scalar(
                        primitive,
                        "pen"),
                    path);
                var layerGuid = JsonOutputHelp.WrappedGuidOf(
                    primitive["layer"],
                    "attributeId");
                Layers.Add(
                    layerGuid == null
                        ? null
                        : new AttributeGuidWrapper
                        {
                            AttributeId =
                                new AttributeGuidObject { Guid = layerGuid }
                        },
                    path);
                DrawIndices.Add(
                    JsonOutputHelp.Scalar(
                        primitive,
                        "drwIndex"),
                    path);
                RenovationStatuses.Add(
                    JsonOutputHelp.Scalar(
                        primitive,
                        "renovationStatus"),
                    path);
                Texts.Add(
                    JsonOutputHelp.Scalar(
                        primitive,
                        "text"),
                    path);
                Primitives.Add(
                    primitive.ToString(Formatting.None),
                    path);
            }
        }

        // The geometry of a primitive in the XY plane: a point for Point and
        // Text (its location), a line, an arc or ellipse, a polyline, a planar
        // surface for Poly (outline with holes) and the destination box of a
        // Pict. Null for the control items.
        public static object GeometryOf(
            JToken primitive)
        {
            switch (primitive?["type"]?.ToString())
            {
                case "Point":
                case "Text":
                    return ElementGeometryJsonHelp.Point2D(primitive["loc"]);
                case "Line":
                    return new LineCurve(
                        ElementGeometryJsonHelp.Point2D(primitive["c1"]),
                        ElementGeometryJsonHelp.Point2D(primitive["c2"]));
                case "Arc":
                    return ArcOf(primitive);
                case "PLine":
                    return ElementGeometryJsonHelp.Polyline2D(
                        primitive["coordinates"],
                        false);
                case "Tri":
                    return ElementGeometryJsonHelp.Polyline2D(
                        primitive["coordinates"],
                        true);
                case "Poly":
                    return PolyOf(primitive);
                case "Pict":
                    return PictOf(primitive);
                default:
                    return null;
            }
        }

        private static Curve ArcOf(
            JToken primitive)
        {
            var radius = primitive.Value<double?>("r") ?? 0.0;
            if (radius <= 0.0)
            {
                return null;
            }

            // The main axis of the ellipse is rotated by angle, the other one
            // is scaled by ratio; the begin and end angles are measured from
            // the main axis.
            var plane = Plane.WorldXY;
            plane.Origin = ElementGeometryJsonHelp.Point2D(primitive["orig"]);
            plane.Rotate(
                primitive.Value<double?>("angle") ?? 0.0,
                Vector3d.ZAxis);

            Curve curve;
            if (primitive.Value<bool?>("whole") == true)
            {
                curve = new ArcCurve(
                    new Circle(
                        plane,
                        radius));
            }
            else
            {
                var begAngle = primitive.Value<double?>("begAng") ?? 0.0;
                var endAngle = primitive.Value<double?>("endAng") ?? 0.0;
                if (primitive.Value<bool?>("reflected") == true)
                {
                    // Clockwise from begin to end is counterclockwise from end
                    // to begin.
                    var swap = begAngle;
                    begAngle = endAngle;
                    endAngle = swap;
                }
                while (endAngle <= begAngle)
                {
                    endAngle += 2.0 * Math.PI;
                }
                curve = new ArcCurve(
                    new Arc(
                        new Circle(
                            plane,
                            radius),
                        new Interval(
                            begAngle,
                            endAngle)));
            }

            var ratio = primitive.Value<double?>("ratio") ?? 1.0;
            if (ratio > 0.0 && Math.Abs(ratio - 1.0) > RhinoMath.ZeroTolerance)
            {
                var nurbs = curve.ToNurbsCurve();
                nurbs.Transform(
                    Transform.Scale(
                        plane,
                        1.0,
                        ratio,
                        1.0));
                return nurbs;
            }
            return curve;
        }

        private static object PolyOf(
            JToken primitive)
        {
            if (!(primitive["polygons"] is JArray polygons))
            {
                return null;
            }

            var curves = polygons
                .Select(p => (Curve)ElementGeometryJsonHelp.Polyline2D(
                    p["coordinates"],
                    true))
                .Where(c => c != null)
                .ToList();
            if (curves.Count == 0)
            {
                return null;
            }

            var breps = Brep.CreatePlanarBreps(
                curves,
                RhinoMath.SqrtEpsilon);
            if (breps != null && breps.Length == 1)
            {
                return breps[0];
            }
            return curves[0];
        }

        private static object PictOf(
            JToken primitive)
        {
            var box = primitive["destBox"];
            if (box == null)
            {
                return null;
            }
            return new Rectangle3d(
                Plane.WorldXY,
                new Point3d(
                    box.Value<double?>("xMin") ?? 0.0,
                    box.Value<double?>("yMin") ?? 0.0,
                    0.0),
                new Point3d(
                    box.Value<double?>("xMax") ?? 0.0,
                    box.Value<double?>("yMax") ?? 0.0,
                    0.0)).ToNurbsCurve();
        }
    }
}
