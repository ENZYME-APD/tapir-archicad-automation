using System;
using System.Collections.Generic;
using System.Linq;
using Grasshopper.Kernel.Types;
using Rhino.Collections;
using Rhino.Geometry;
using TapirGrasshopperPlugin.Components.ElementsComponents;

namespace TapirGrasshopperPlugin.Utilities
{
    public class Convert
    {
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

        static public Polyline ToPolygon (List<Point2D> points, double zCoordinate)
        {
            Point3dList polygon = new Point3dList ();
            foreach (Point2D point2D in points) {
                polygon.Add (new Point3d (point2D.X, point2D.Y, zCoordinate));
            }
            polygon.Add (polygon.First);
            return new Polyline (polygon);
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
