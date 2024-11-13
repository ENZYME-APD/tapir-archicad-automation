using System;
using System.Collections.Generic;
using System.Linq;
using Grasshopper.Kernel.Types;

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
}
