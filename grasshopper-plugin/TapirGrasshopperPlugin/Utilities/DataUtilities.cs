using System;
using System.Collections.Generic;
using System.Linq;
using Grasshopper.Kernel.Types;
using Rhino.Geometry;
using TapirGrasshopperPlugin.ResponseTypes.Element;
using Arc = TapirGrasshopperPlugin.ResponseTypes.Element.Arc;

namespace TapirGrasshopperPlugin.Utilities
{
    public class Convert
    {
        public const double Epsilon = 1E-5;

        public static List<int> ToRgb(
            GH_Colour color,
            int alpha)
        {
            return color == null
                ? null
                : new List<int>
                {
                    color.Value.R, color.Value.G, color.Value.B, alpha
                };
        }

        public static List<List<int>> ToRgb(
            List<GH_Colour> ghColors,
            int alpha)
        {
            return ghColors
                ?.Select(x => ToRgb(
                    x,
                    alpha))
                .ToList();
        }

        public static Point2d GetArcOrigo(
            Point2d begC,
            Point2d endC,
            double angle)
        {
            var origo = new Point2d();
            double xm, ym, m;

            xm = begC.X + endC.X;
            ym = begC.Y + endC.Y;
            if (Math.Abs(Math.Abs(angle) - Math.PI) < Epsilon)
            {
                origo.X = xm / 2;
                origo.Y = ym / 2;
            }
            else
            {
                m = 1.0 / Math.Tan(angle / 2.0);
                origo.X = (xm - m * (endC.Y - begC.Y)) / 2;
                origo.Y = (ym + m * (endC.X - begC.X)) / 2;
            }

            return origo;
        }

        public static PolyCurve ToPolyCurve(
            List<Point2D> points,
            List<Arc> arcs,
            double zCoordinate,
            double? slantAngle = null,
            double? verticalCurveHeight = null)
        {
            var polyCurve = new PolyCurve();
            var iArc = 0;
            for (var i = 0; i < points.Count - 1; ++i)
            {
                var begPoint2D = new Point2d(
                    points[i].X,
                    points[i].Y);
                var endPoint2D = new Point2d(
                    points[i + 1].X,
                    points[i + 1].Y);
                var begPoint3D = new Point3d(
                    begPoint2D.X,
                    begPoint2D.Y,
                    zCoordinate);
                var endPoint3D = new Point3d(
                    endPoint2D.X,
                    endPoint2D.Y,
                    zCoordinate);
                if (slantAngle.HasValue &&
                    Math.Abs(slantAngle.Value) >= Epsilon)
                {
                    var dirV = endPoint3D - begPoint3D;
                    var delta = Math.Sin(slantAngle.Value) * dirV.Length;
                    endPoint3D.Z += delta;
                }

                if (arcs != null && iArc < arcs.Count &&
                    arcs[iArc].BegIndex == i)
                {
                    var arcAngle = arcs[iArc].arcAngle;
                    var arcOrigo = GetArcOrigo(
                        begPoint2D,
                        endPoint2D,
                        arcAngle);
                    var radius = new Point2d(
                        arcOrigo.X,
                        arcOrigo.Y).DistanceTo(
                        new Point2d(
                            begPoint2D.X,
                            begPoint2D.Y));
                    var arcOrigo3D = new Point3d(
                        arcOrigo.X,
                        arcOrigo.Y,
                        zCoordinate);
                    var dirV = endPoint2D - begPoint2D;
                    Vector3d normalV3D;
                    if (arcAngle < 0.0)
                    {
                        normalV3D = new Vector3d(
                            -dirV.Y,
                            dirV.X,
                            0);
                    }
                    else
                    {
                        normalV3D = new Vector3d(
                            dirV.Y,
                            -dirV.X,
                            0);
                    }

                    normalV3D.Unitize();
                    var interiorPoint3D = arcOrigo3D + normalV3D * radius;
                    var arc = new Rhino.Geometry.Arc(
                        begPoint3D,
                        interiorPoint3D,
                        endPoint3D);
                    polyCurve.Append(arc);
                    iArc++;
                }
                else if (verticalCurveHeight.HasValue &&
                         Math.Abs(verticalCurveHeight.Value) >= Epsilon)
                {
                    var midPoint = new Point3d(
                        (begPoint3D.X + endPoint3D.X) / 2,
                        (begPoint3D.Y + endPoint3D.Y) / 2,
                        (begPoint3D.Z + endPoint3D.Z) / 2);
                    var dirV = endPoint2D - begPoint2D;
                    var normalV3D = new Vector3d(
                        -dirV.Y,
                        dirV.X,
                        0);
                    normalV3D.Unitize();
                    var interiorPoint3D =
                        midPoint + normalV3D * verticalCurveHeight.Value;
                    var arc = new Rhino.Geometry.Arc(
                        begPoint3D,
                        interiorPoint3D,
                        endPoint3D);
                    var axis = endPoint3D - begPoint3D;
                    axis.Unitize();
                    arc.Transform(
                        Transform.Rotation(
                            Math.PI / 2,
                            axis,
                            midPoint));
                    polyCurve.Append(arc);
                }
                else
                {
                    polyCurve.Append(
                        new Line(
                            begPoint3D,
                            endPoint3D));
                }
            }

            return polyCurve;
        }

        public static PolyCurve ToPolyCurve(
            List<Point3D> points)
        {
            var polyCurve = new PolyCurve();
            for (var i = 0; i < points.Count - 1; ++i)
            {
                var begPoint3D = new Point3d(
                    points[i].X,
                    points[i].Y,
                    points[i].Z);
                var endPoint3D = new Point3d(
                    points[i + 1].X,
                    points[i + 1].Y,
                    points[i + 1].Z);

                polyCurve.Append(
                    new Line(
                        begPoint3D,
                        endPoint3D));
            }

            return polyCurve;
        }
    }

    static class Extensions
    {
        public static bool IsValid<T>(
            this string s)
            where T : Enum
        {
            foreach (T f in Enum.GetValues(typeof(T)))
            {
                if (f.ToString() == s)
                {
                    return true;
                }
            }

            return false;
        }
    }

    public class AcceptsElementFilters
    {
        public List<string> AcceptElementFilters(
            List<string> filters)
        {
            filters.RemoveAll(f =>
                f == ElementFilter.NoFilter.ToString() ||
                !f.IsValid<ElementFilter>());
            if (filters.Count == 0)
            {
                return null;
            }

            return filters;
        }
    }
}