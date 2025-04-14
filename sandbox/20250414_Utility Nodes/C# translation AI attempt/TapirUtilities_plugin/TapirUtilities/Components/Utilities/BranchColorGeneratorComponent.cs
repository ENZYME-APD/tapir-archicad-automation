using Grasshopper.Kernel;
using System;
using System.Drawing;
using System.Collections.Generic;
using System.Linq;
using System.Drawing.Drawing2D;

namespace Tapir.Components.Utilities
{
    public class BranchColorGeneratorComponent : Component
    {
        /// <summary>
        /// Initializes a new instance of the BranchColorGeneratorComponent class.
        /// </summary>
        public BranchColorGeneratorComponent()
            : base("Branch Color Generator", "BranchColorGen", 
                  "Generates colors for branches in a data tree", "Utilities")
        {
        }

        /// <summary>
        /// Registers all the input parameters for this component.
        /// </summary>
        protected override void RegisterInputParams(GH_Component.GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter("Tree", "T", "A Grasshopper DataTree with multiple branches", GH_ParamAccess.tree);
            pManager.AddColourParameter("Initial Color", "IC", "Starting color for gradient or base color for complementary mode", GH_ParamAccess.item);
            pManager[1].Optional = true;
            pManager.AddColourParameter("End Color", "EC", "Ending color for gradient mode", GH_ParamAccess.item);
            pManager[2].Optional = true;
            pManager.AddIntegerParameter("Seed", "S", "Seed value for random color generation", GH_ParamAccess.item);
            pManager[3].Optional = true;
            pManager.AddBooleanParameter("Flatten", "F", "Whether outputs are flattened", GH_ParamAccess.item, false);
            pManager[4].Optional = true;
            pManager.AddTextParameter("Mode", "M", "Color generation mode: 'gradient', 'complementary', or 'random'", GH_ParamAccess.item, "random");
            pManager[5].Optional = true;
        }

        /// <summary>
        /// Registers all the output parameters for this component.
        /// </summary>
        protected override void RegisterOutputParams(GH_Component.GH_OutputParamManager pManager)
        {
            pManager.AddColourParameter("Colors", "C", "Generated colors for each branch", GH_ParamAccess.tree);
            pManager.AddGenericParameter("Input Tree", "T", "Input tree with original structure", GH_ParamAccess.tree);
        }

        /// <summary>
        /// This is the method that actually does the work.
        /// </summary>
        /// <param name="DA">The DA object is used to retrieve from inputs and store in outputs.</param>
        protected override void SolveInstance(IGH_DataAccess DA)
        {
            // Initialize default message
            string message = "Waiting for input DataTree.";

            // Get input tree
            Grasshopper.Kernel.Data.GH_Structure<Grasshopper.Kernel.Types.IGH_Goo> tree = new Grasshopper.Kernel.Data.GH_Structure<Grasshopper.Kernel.Types.IGH_Goo>();
            if (!DA.GetDataTree(0, out tree) || tree.Branches.Count == 0)
            {
                this.Message = message;
                return;
            }

            // Get optional inputs
            Color initColor = Color.Black;
            DA.GetData(1, ref initColor);

            Color endColor = Color.White;
            DA.GetData(2, ref endColor);

            int? seed = null;
            int seedValue = 0;
            if (DA.GetData(3, ref seedValue))
            {
                seed = seedValue;
            }

            bool flatten = false;
            DA.GetData(4, ref flatten);

            string mode = "random";
            DA.GetData(5, ref mode);

            // Process the input DataTree
            int numBranches = tree.Branches.Count;
            
            // Initialize output trees
            Grasshopper.Kernel.Data.GH_Structure<Grasshopper.Kernel.Types.GH_Colour> colors = new Grasshopper.Kernel.Data.GH_Structure<Grasshopper.Kernel.Types.GH_Colour>();
            Grasshopper.Kernel.Data.GH_Structure<Grasshopper.Kernel.Types.IGH_Goo> inputTree = new Grasshopper.Kernel.Data.GH_Structure<Grasshopper.Kernel.Types.IGH_Goo>();

            // Copy the input tree to the output
            for (int i = 0; i < tree.Branches.Count; i++)
            {
                var path = tree.Paths[i];
                var branchData = tree.Branches[i];
                inputTree.AppendRange(branchData, path);
            }

            // Generate colors based on selected mode
            List<Color> branchColors = new List<Color>();
            string selectedMode = GetMode(mode);
            string modeMessage = "";

            if (selectedMode == "gradient" && numBranches > 0)
            {
                for (int i = 0; i < numBranches; i++)
                {
                    double t = numBranches > 1 ? (double)i / (numBranches - 1) : 0;
                    Color color = InterpolateColor(initColor, endColor, t);
                    branchColors.Add(color);
                }
                modeMessage = "Mode: gradient";
            }
            else if (selectedMode == "complementary" && numBranches > 0)
            {
                branchColors = GetDistributedColors(initColor, numBranches);
                modeMessage = "Mode: complementary";
            }
            else // Default to random mode
            {
                HashSet<Color> usedColors = new HashSet<Color>();
                Random random = seed.HasValue ? new Random(seed.Value) : new Random();
                
                for (int i = 0; i < numBranches; i++)
                {
                    Color color = RandomColor(usedColors, random);
                    branchColors.Add(color);
                    usedColors.Add(color);
                }
                modeMessage = "Mode: random color";
            }

            // Populate the colors DataTree to mirror the input tree structure
            for (int i = 0; i < numBranches; i++)
            {
                var path = tree.Paths[i];
                var branchData = tree.Branches[i];
                
                // Create a list of GH_Colour objects for each item in the branch
                List<Grasshopper.Kernel.Types.GH_Colour> colorList = new List<Grasshopper.Kernel.Types.GH_Colour>();
                for (int j = 0; j < branchData.Count; j++)
                {
                    colorList.Add(new Grasshopper.Kernel.Types.GH_Colour(branchColors[i]));
                }
                
                colors.AppendRange(colorList, path);
            }

            // Handle flattening based on the flatten input
            string flattenMessage = flatten ? "output flattened" : "keep datatree";
            if (flatten)
            {
                // Flatten colors
                Grasshopper.Kernel.Data.GH_Structure<Grasshopper.Kernel.Types.GH_Colour> flatColors = new Grasshopper.Kernel.Data.GH_Structure<Grasshopper.Kernel.Types.GH_Colour>();
                List<Grasshopper.Kernel.Types.GH_Colour> allColors = new List<Grasshopper.Kernel.Types.GH_Colour>();
                
                foreach (var branch in colors.Branches)
                {
                    allColors.AddRange(branch);
                }
                
                flatColors.AppendRange(allColors, new Grasshopper.Kernel.Data.GH_Path(0));
                colors = flatColors;
                
                // Flatten inputTree
                Grasshopper.Kernel.Data.GH_Structure<Grasshopper.Kernel.Types.IGH_Goo> flatInput = new Grasshopper.Kernel.Data.GH_Structure<Grasshopper.Kernel.Types.IGH_Goo>();
                List<Grasshopper.Kernel.Types.IGH_Goo> allData = new List<Grasshopper.Kernel.Types.IGH_Goo>();
                
                foreach (var branch in inputTree.Branches)
                {
                    allData.AddRange(branch);
                }
                
                flatInput.AppendRange(allData, new Grasshopper.Kernel.Data.GH_Path(0));
                inputTree = flatInput;
            }

            // Update message with mode and flatten info
            message = $"{modeMessage}\n{flattenMessage}\n{numBranches} colors for {numBranches} branches";

            // Set outputs
            DA.SetDataTree(0, colors);
            DA.SetDataTree(1, inputTree);

            // Update component message
            this.Message = message;
        }

        /// <summary>
        /// Generates a random color that is not in the used colors set.
        /// </summary>
        private Color RandomColor(HashSet<Color> usedColors, Random random)
        {
            while (true)
            {
                int r = random.Next(0, 256);
                int g = random.Next(0, 256);
                int b = random.Next(0, 256);
                Color color = Color.FromArgb(r, g, b);
                
                // Ensure the color is unique (or close enough)
                if (!usedColors.Contains(color))
                {
                    return color;
                }
            }
        }

        /// <summary>
        /// Interpolates between two colors.
        /// </summary>
        private Color InterpolateColor(Color startColor, Color endColor, double t)
        {
            int r = (int)(startColor.R + (endColor.R - startColor.R) * t);
            int g = (int)(startColor.G + (endColor.G - startColor.G) * t);
            int b = (int)(startColor.B + (endColor.B - startColor.B) * t);
            return Color.FromArgb(r, g, b);
        }

        /// <summary>
        /// Converts RGB color to HSV.
        /// </summary>
        private (double h, double s, double v) RgbToHsv(Color color)
        {
            double r = color.R / 255.0;
            double g = color.G / 255.0;
            double b = color.B / 255.0;
            
            double max = Math.Max(r, Math.Max(g, b));
            double min = Math.Min(r, Math.Min(g, b));
            double delta = max - min;
            
            double h = 0;
            if (delta != 0)
            {
                if (max == r)
                    h = ((g - b) / delta) % 6;
                else if (max == g)
                    h = (b - r) / delta + 2;
                else
                    h = (r - g) / delta + 4;
                
                h *= 60;
                if (h < 0)
                    h += 360;
                h /= 360; // Normalize to [0, 1]
            }
            
            double s = max == 0 ? 0 : delta / max;
            double v = max;
            
            return (h, s, v);
        }

        /// <summary>
        /// Converts HSV color to RGB.
        /// </summary>
        private Color HsvToRgb(double h, double s, double v)
        {
            double r, g, b;
            
            if (s == 0)
            {
                r = g = b = v;
            }
            else
            {
                h *= 6; // [0, 1] -> [0, 6]
                int i = (int)Math.Floor(h);
                double f = h - i;
                double p = v * (1 - s);
                double q = v * (1 - s * f);
                double t = v * (1 - s * (1 - f));
                
                switch (i % 6)
                {
                    case 0: r = v; g = t; b = p; break;
                    case 1: r = q; g = v; b = p; break;
                    case 2: r = p; g = v; b = t; break;
                    case 3: r = p; g = q; b = v; break;
                    case 4: r = t; g = p; b = v; break;
                    default: r = v; g = p; b = q; break;
                }
            }
            
            return Color.FromArgb((int)(r * 255), (int)(g * 255), (int)(b * 255));
        }

        /// <summary>
        /// Normalizes the mode input.
        /// </summary>
        private string GetMode(object modeInput)
        {
            if (modeInput is int modeInt)
            {
                Dictionary<int, string> modeMap = new Dictionary<int, string>
                {
                    { 0, "gradient" },
                    { 1, "complementary" },
                    { 2, "random" }
                };
                return modeMap.ContainsKey(modeInt) ? modeMap[modeInt] : "random";
            }
            else if (modeInput is string modeStr)
            {
                string normalizedMode = modeStr.ToLower().Trim();
                return new[] { "gradient", "complementary", "random" }.Contains(normalizedMode) ? normalizedMode : "random";
            }
            return "random";
        }

        /// <summary>
        /// Generates evenly distributed colors based on initColor.
        /// </summary>
        private List<Color> GetDistributedColors(Color initColor, int numColors)
        {
            List<Color> colorsList = new List<Color>();
            
            // First color is initColor
            colorsList.Add(initColor);
            
            if (numColors > 1)
            {
                // Convert initColor to HSV
                var (h, s, v) = RgbToHsv(initColor);
                
                // Calculate evenly spaced hues starting with the complement
                double hueStep = 1.0 / numColors;
                for (int i = 1; i < numColors; i++)
                {
                    // Offset hue by complement (0.5) and distribute evenly
                    double newHue = (h + 0.5 + i * hueStep) % 1.0;
                    Color color = HsvToRgb(newHue, s, v);
                    colorsList.Add(color);
                }
            }
            
            return colorsList;
        }

        /// <summary>
        /// Provides an Icon for the component.
        /// </summary>
        protected override System.Drawing.Bitmap Icon
        {
            get
            {
                // You can add custom icon here
                return null;
            }
        }

        /// <summary>
        /// Gets the unique ID for this component. Do not change this ID after release.
        /// </summary>
        public override Guid ComponentGuid
        {
            get { return new Guid("A1B11162-627B-455B-B9C0-963E76B36DC1"); }
        }
    }
}
