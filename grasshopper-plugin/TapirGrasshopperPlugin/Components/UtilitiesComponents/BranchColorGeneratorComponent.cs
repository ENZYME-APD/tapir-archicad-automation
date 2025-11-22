using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Grasshopper.Kernel.Data;
using System;
using System.Drawing;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class BranchColorGeneratorComponent : Component
    {
        public static Dictionary<int, string> ModeMap =
            new Dictionary<int, string>
            {
                { 0, "gradient" }, { 1, "complementary" }, { 2, "random" }
            };

        public BranchColorGeneratorComponent()
            : base(
                "BranchColorGenerator",
                "Generates colors for branches in a data tree",
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            InGenericTree(
                "Tree",
                "A Grasshopper DataTree with multiple branches");

            InColor(
                "Starting Color",
                "Starting color for gradient or base color for complementary mode");

            InColor(
                "End Color",
                "Ending color for gradient mode");

            InInteger(
                "Seed",
                "Seed value for random color generation");

            InBoolean(
                "Flatten",
                "Whether outputs are flattened");

            InText(
                "Mode",
                "Color generation mode: 'gradient', 'complementary', or 'random'",
                "random");

            Params.Input[1].Optional = true;
            Params.Input[2].Optional = true;
            Params.Input[3].Optional = true;
            Params.Input[4].Optional = true;
            Params.Input[5].Optional = true;
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddColourParameter(
                "Generated Colors",
                "Colors",
                "Generated colors for each branch",
                GH_ParamAccess.tree);
            pManager.AddGenericParameter(
                "Input Tree",
                "Tree",
                "Input tree with original structure",
                GH_ParamAccess.tree);
        }

        protected override void SolveInstance(
            IGH_DataAccess DA)
        {
            // Initialize default message
            var message = "Waiting for input DataTree.";

            // Get input tree
            var tree = new GH_Structure<IGH_Goo>();
            if (!DA.GetDataTree(
                    0,
                    out tree) || tree.Branches.Count == 0)
            {
                Message = message;
                return;
            }

            var initColor = DA.GetOptionalItem(
                1,
                Color.Black);

            var endColor = DA.GetOptionalItem(
                2,
                Color.White);

            int? seed = null;
            var seedValue = 0;
            if (DA.GetData(
                    3,
                    ref seedValue))
            {
                seed = seedValue;
            }

            var flatten = DA.GetOptionalItem(
                4,
                false);

            var mode = DA.GetOptionalItem(
                5,
                "random");

            // Process the input DataTree
            var numBranches = tree.Branches.Count;

            // Initialize output trees
            var colors = new GH_Structure<GH_Colour>();
            var inputTree = new GH_Structure<IGH_Goo>();

            // Copy the input tree to the output
            for (var i = 0; i < tree.Branches.Count; i++)
            {
                var path = tree.Paths[i];
                var branchData = tree.Branches[i];
                inputTree.AppendRange(
                    branchData,
                    path);
            }

            // Generate colors based on selected mode
            var branchColors = new List<Color>();
            var selectedMode = GetMode(mode);
            var modeMessage = "";

            if (selectedMode == "gradient" && numBranches > 0)
            {
                for (var i = 0; i < numBranches; i++)
                {
                    var t = numBranches > 1 ? (double)i / (numBranches - 1) : 0;
                    var color = InterpolateColor(
                        initColor,
                        endColor,
                        t);
                    branchColors.Add(color);
                }

                modeMessage = "Mode: gradient";
            }
            else if (selectedMode == "complementary" && numBranches > 0)
            {
                branchColors = GetDistributedColors(
                    initColor,
                    numBranches);
                modeMessage = "Mode: complementary";
            }
            else // Default to random mode
            {
                var usedColors = new HashSet<Color>();
                var random = seed.HasValue
                    ? new Random(seed.Value)
                    : new Random();

                for (var i = 0; i < numBranches; i++)
                {
                    var color = RandomColor(
                        usedColors,
                        random);
                    branchColors.Add(color);
                    usedColors.Add(color);
                }

                modeMessage = "Mode: random color";
            }

            // Populate the colors DataTree to mirror the input tree structure
            for (var i = 0; i < numBranches; i++)
            {
                var path = tree.Paths[i];
                var branchData = tree.Branches[i];

                // Create a list of GH_Colour objects for each item in the branch
                var colorList = new List<GH_Colour>();
                for (var j = 0; j < branchData.Count; j++)
                {
                    colorList.Add(new GH_Colour(branchColors[i]));
                }

                colors.AppendRange(
                    colorList,
                    path);
            }

            // Handle flattening based on the flatten input
            var flattenMessage = flatten ? "output flattened" : "keep datatree";
            if (flatten)
            {
                // Flatten colors
                var flatColors = new GH_Structure<GH_Colour>();
                var allColors = new List<GH_Colour>();

                foreach (var branch in colors.Branches)
                {
                    allColors.AddRange(branch);
                }

                flatColors.AppendRange(
                    allColors,
                    new GH_Path(0));
                colors = flatColors;

                // Flatten inputTree
                var flatInput = new GH_Structure<IGH_Goo>();
                var allData = new List<IGH_Goo>();

                foreach (var branch in inputTree.Branches)
                {
                    allData.AddRange(branch);
                }

                flatInput.AppendRange(
                    allData,
                    new GH_Path(0));
                inputTree = flatInput;
            }

            // Update message with mode and flatten info
            message =
                $"{modeMessage}\n{flattenMessage}\n{numBranches} colors for {numBranches} branches";

            // Set outputs
            DA.SetDataTree(
                0,
                colors);
            DA.SetDataTree(
                1,
                inputTree);

            // Update component message
            Message = message;
        }

        private Color RandomColor(
            HashSet<Color> usedColors,
            Random random)
        {
            while (true)
            {
                var r = random.Next(
                    0,
                    256);
                var g = random.Next(
                    0,
                    256);
                var b = random.Next(
                    0,
                    256);
                var color = Color.FromArgb(
                    r,
                    g,
                    b);

                // Ensure the color is unique (or close enough)
                if (!usedColors.Contains(color))
                {
                    return color;
                }
            }
        }

        private Color InterpolateColor(
            Color startColor,
            Color endColor,
            double t)
        {
            var r = (int)(startColor.R + (endColor.R - startColor.R) * t);
            var g = (int)(startColor.G + (endColor.G - startColor.G) * t);
            var b = (int)(startColor.B + (endColor.B - startColor.B) * t);
            return Color.FromArgb(
                r,
                g,
                b);
        }

        private (double h, double s, double v) RgbToHsv(
            Color color)
        {
            var r = color.R / 255.0;
            var g = color.G / 255.0;
            var b = color.B / 255.0;

            var max = Math.Max(
                r,
                Math.Max(
                    g,
                    b));
            var min = Math.Min(
                r,
                Math.Min(
                    g,
                    b));
            var delta = max - min;

            double h = 0;
            if (delta != 0)
            {
                if (max == r)
                {
                    h = ((g - b) / delta) % 6;
                }
                else if (max == g)
                {
                    h = (b - r) / delta + 2;
                }
                else
                {
                    h = (r - g) / delta + 4;
                }

                h *= 60;
                if (h < 0)
                {
                    h += 360;
                }

                h /= 360; // Normalize to [0, 1]
            }

            var s = max == 0 ? 0 : delta / max;
            var v = max;

            return (h, s, v);
        }

        private Color HsvToRgb(
            double h,
            double s,
            double v)
        {
            double r, g, b;

            if (s == 0)
            {
                r = g = b = v;
            }
            else
            {
                h *= 6; // [0, 1] -> [0, 6]
                var i = (int)Math.Floor(h);
                var f = h - i;
                var p = v * (1 - s);
                var q = v * (1 - s * f);
                var t = v * (1 - s * (1 - f));

                switch (i % 6)
                {
                    case 0:
                        r = v;
                        g = t;
                        b = p;
                        break;
                    case 1:
                        r = q;
                        g = v;
                        b = p;
                        break;
                    case 2:
                        r = p;
                        g = v;
                        b = t;
                        break;
                    case 3:
                        r = p;
                        g = q;
                        b = v;
                        break;
                    case 4:
                        r = t;
                        g = p;
                        b = v;
                        break;
                    default:
                        r = v;
                        g = p;
                        b = q;
                        break;
                }
            }

            return Color.FromArgb(
                (int)(r * 255),
                (int)(g * 255),
                (int)(b * 255));
        }

        private string GetMode(
            object modeInput)
        {
            if (modeInput is int modeInt)
            {
                string modeStr;
                if (ModeMap.TryGetValue(
                        modeInt,
                        out modeStr))
                {
                    return modeStr;
                }
            }
            else if (modeInput is string modeStr)
            {
                var normalizedMode = modeStr.ToLower().Trim();
                if (ModeMap.ContainsValue(normalizedMode))
                {
                    return normalizedMode;
                }
            }

            return "random";
        }

        private List<Color> GetDistributedColors(
            Color initColor,
            int numColors)
        {
            var colorsList = new List<Color>();

            // First color is initColor
            colorsList.Add(initColor);

            if (numColors > 1)
            {
                // Convert initColor to HSV
                var (h, s, v) = RgbToHsv(initColor);

                // Calculate evenly spaced hues starting with the complement
                var hueStep = 1.0 / numColors;
                for (var i = 1; i < numColors; i++)
                {
                    // Offset hue by complement (0.5) and distribute evenly
                    var newHue = (h + 0.5 + i * hueStep) % 1.0;
                    var color = HsvToRgb(
                        newHue,
                        s,
                        v);
                    colorsList.Add(color);
                }
            }

            return colorsList;
        }

        protected override Bitmap Icon =>
            Properties.Resources.BranchColorGenerator;

        public override Guid ComponentGuid =>
            new Guid("A1B11162-627B-455B-B9C0-963E76B36DC1");
    }
}