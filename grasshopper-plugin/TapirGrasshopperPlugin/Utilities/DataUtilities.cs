using System;
using System.Collections.Generic;
using System.Linq;
using Grasshopper.Kernel.Types;

namespace TapirGrasshopperPlugin.Utilities
{
    public class Convert
    {
        static public List<int> ToColor (GH_Colour colorRGBA)
        {
            if (colorRGBA == null) {
                return null;
            }
            return new List<int> {
                colorRGBA.Value.R,
                colorRGBA.Value.G,
                colorRGBA.Value.B,
                colorRGBA.Value.A
            };
        }

        static public List<List<int>> ToColors (List<GH_Colour> colorRGBAs, int minSize = 1)
        {
            if (colorRGBAs == null) {
                return null;
            }
            List<List<int>> colors = new List<List<int>> ();
            foreach (GH_Colour colorRGBA in colorRGBAs) {
                colors.Add (ToColor (colorRGBA));
            }
            for (int i = colors.Count; i < minSize; ++i) {
                colors.Add (colors.Last ());
            }
            return colors;
        }
    }
}
