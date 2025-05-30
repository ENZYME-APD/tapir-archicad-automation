using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using Grasshopper.Kernel.Types;
using Rhino.Collections;
using Rhino.Geometry;
using TapirGrasshopperPlugin.Components.ElementsComponents;

namespace TapirGrasshopperPlugin.Utilities
{
    public class Convert
    {
        public const double Epsilon = 1E-5;

        static public List<int> ToRGBColor (GH_Colour colorRGBA, int alpha)
        {
            if (colorRGBA == null) {
                return null;
            }
            return new List<int> {
                colorRGBA.Value.R,
                colorRGBA.Value.G,
                colorRGBA.Value.B,
                alpha
            };
        }

        static public List<List<int>> ToRGBColors (List<GH_Colour> colorRGBAs, int alpha, int minSize)
        {
            if (colorRGBAs == null) {
                return null;
            }
            List<List<int>> colors = new List<List<int>> ();
            foreach (GH_Colour colorRGBA in colorRGBAs) {
                colors.Add (ToRGBColor (colorRGBA, alpha));
            }
            for (int i = colors.Count; i < minSize; ++i) {
                colors.Add (colors.Last ());
            }
            return colors;
        }

        static public Point2d GetArcOrigo (
            Point2d begC,
            Point2d endC,
            double angle)
        {
            Point2d origo = new Point2d ();
            double xm, ym, m;

            xm = begC.X + endC.X;
            ym = begC.Y + endC.Y;
            if (Math.Abs (Math.Abs (angle) - Math.PI) < Epsilon) {
                origo.X = xm / 2;
                origo.Y = ym / 2;
            } else {
                m = 1.0 / Math.Tan (angle / 2.0);
                origo.X = (xm - m * (endC.Y - begC.Y)) / 2;
                origo.Y = (ym + m * (endC.X - begC.X)) / 2;
            }

            return origo;
        }

        static public PolyCurve ToPolyCurve (List<Point2D> points, List<Components.ElementsComponents.Arc> arcs, double zCoordinate, double? slantAngle = null, double? verticalCurveHeight = null)
        {
            PolyCurve polyCurve = new PolyCurve ();
            int iArc = 0;
            for (int i = 0; i < points.Count - 1; ++i) {
                Point2d begPoint2D = new Point2d (points[i].X, points[i].Y);
                Point2d endPoint2D = new Point2d (points[i + 1].X, points[i + 1].Y);
                Point3d begPoint3D = new Point3d (begPoint2D.X, begPoint2D.Y, zCoordinate);
                Point3d endPoint3D = new Point3d (endPoint2D.X, endPoint2D.Y, zCoordinate);
                if (slantAngle.HasValue && Math.Abs (slantAngle.Value) >= Epsilon) {
                    Vector3d dirV = endPoint3D - begPoint3D;
                    double delta = Math.Sin (slantAngle.Value) * dirV.Length;
                    endPoint3D.Z += delta;
                }

                if (arcs != null && iArc < arcs.Count && arcs[iArc].BegIndex == i) {
                    double arcAngle = arcs[iArc].arcAngle;
                    Point2d arcOrigo = GetArcOrigo (begPoint2D, endPoint2D, arcAngle);
                    double radius = new Point2d (arcOrigo.X, arcOrigo.Y).DistanceTo (new Point2d (begPoint2D.X, begPoint2D.Y));
                    Point3d arcOrigo3D = new Point3d (arcOrigo.X, arcOrigo.Y, zCoordinate);
                    Vector2d dirV = endPoint2D - begPoint2D;
                    Vector3d normalV3D;
                    if (arcAngle < 0.0) {
                        normalV3D = new Vector3d (-dirV.Y, dirV.X, 0);
                    } else {
                        normalV3D = new Vector3d (dirV.Y, -dirV.X, 0);
                    }
                    normalV3D.Unitize ();
                    Point3d interiorPoint3D = arcOrigo3D + normalV3D * radius;
                    Rhino.Geometry.Arc arc = new Rhino.Geometry.Arc (begPoint3D, interiorPoint3D, endPoint3D);
                    polyCurve.Append (arc);
                    iArc++;
                } else if (verticalCurveHeight.HasValue && Math.Abs (verticalCurveHeight.Value) >= Epsilon) {
                    Point3d midPoint = new Point3d ((begPoint3D.X + endPoint3D.X) / 2, (begPoint3D.Y + endPoint3D.Y) / 2, (begPoint3D.Z + endPoint3D.Z) / 2);
                    Vector2d dirV = endPoint2D - begPoint2D;
                    Vector3d normalV3D = new Vector3d (-dirV.Y, dirV.X, 0);
                    normalV3D.Unitize ();
                    Point3d interiorPoint3D = midPoint + normalV3D * verticalCurveHeight.Value;
                    Rhino.Geometry.Arc arc = new Rhino.Geometry.Arc (begPoint3D, interiorPoint3D, endPoint3D);
                    Vector3d axis = endPoint3D - begPoint3D;
                    axis.Unitize ();
                    arc.Transform (Transform.Rotation (Math.PI / 2, axis, midPoint));
                    polyCurve.Append (arc);
                } else {
                    polyCurve.Append (new Line (new Point3d (begPoint2D.X, begPoint2D.Y, zCoordinate), new Point3d (endPoint2D.X, endPoint2D.Y, zCoordinate)));
                }
            }
            return polyCurve;
        }
    }

    static class Extensions
    {
        public static bool IsValid<T> (this string s) where T : Enum
        {
            foreach (T f in Enum.GetValues (typeof (T))) {
                if (f.ToString () == s) {
                    return true;
                }
            }

            return false;
        }
    }

    public class AcceptsElementFilters
    {
        public List<string> AcceptElementFilters (List<string> filters)
        {
            filters.RemoveAll (f => f == ElementFilter.NoFilter.ToString () || !f.IsValid<ElementFilter> ());
            if (filters.Count == 0) {
                return null;
            }
            return filters;
        }
    }
}
