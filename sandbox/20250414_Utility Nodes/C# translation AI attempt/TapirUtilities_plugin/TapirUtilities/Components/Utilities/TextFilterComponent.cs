using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;

namespace Tapir.Components.Utilities
{
    public class TextFilterComponent : Component
    {
        /// <summary>
        /// Initializes a new instance of the TextFilterComponent class.
        /// </summary>
        public TextFilterComponent()
            : base("Filter Contains String", "Filter Contains", 
                  "Filters a list of strings based on a search string", "Utilities")
        {
        }

        /// <summary>
        /// Registers all the input parameters for this component.
        /// </summary>
        protected override void RegisterInputParams(GH_Component.GH_InputParamManager pManager)
        {
            pManager.AddTextParameter("Strings List", "L", "A list of strings to search through", GH_ParamAccess.list);
            pManager.AddTextParameter("Search String", "S", "The string to search for within the strings list", GH_ParamAccess.item);
            pManager.AddBooleanParameter("Case Sensitive", "C", "Determines if the search is case-sensitive", GH_ParamAccess.item, false);
            pManager[2].Optional = true;
            pManager.AddBooleanParameter("Whole Words", "W", "Determines if the search matches whole words only", GH_ParamAccess.item, true);
            pManager[3].Optional = true;
        }

        /// <summary>
        /// Registers all the output parameters for this component.
        /// </summary>
        protected override void RegisterOutputParams(GH_Component.GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter("Filtered List", "F", "List of strings that match the search criteria", GH_ParamAccess.list);
            pManager.AddIntegerParameter("Filtered Indices", "I", "List of indices of the matched strings", GH_ParamAccess.list);
            pManager.AddIntegerParameter("Count", "C", "Number of matches", GH_ParamAccess.item);
            pManager.AddBooleanParameter("Pattern", "P", "Boolean list indicating which strings matched", GH_ParamAccess.list);
        }

        /// <summary>
        /// This is the method that actually does the work.
        /// </summary>
        /// <param name="DA">The DA object is used to retrieve from inputs and store in outputs.</param>
        protected override void SolveInstance(IGH_DataAccess DA)
        {
            // Initialize outputs
            List<string> filteredList = new List<string>();
            List<int> filteredIndices = new List<int>();
            int count = 0;
            List<bool> pattern = new List<bool>();
            string message = "Waiting for input strings.";

            // Get inputs
            List<string> stringsList = new List<string>();
            if (!DA.GetDataList(0, stringsList) || stringsList.Count == 0)
            {
                message = "Error: Provide both a strings list and a search string.";
                this.Message = message;
                DA.SetDataList(0, filteredList);
                DA.SetDataList(1, filteredIndices);
                DA.SetData(2, count);
                DA.SetDataList(3, pattern);
                return;
            }

            string searchStr = "";
            if (!DA.GetData(1, ref searchStr) || string.IsNullOrEmpty(searchStr))
            {
                message = "Error: Provide both a strings list and a search string.";
                this.Message = message;
                DA.SetDataList(0, filteredList);
                DA.SetDataList(1, filteredIndices);
                DA.SetData(2, count);
                DA.SetDataList(3, pattern);
                return;
            }

            bool caseSensitive = false;
            DA.GetData(2, ref caseSensitive);

            bool wholeWords = true;
            DA.GetData(3, ref wholeWords);

            try
            {
                // Initialize pattern with false for all items
                pattern = Enumerable.Repeat(false, stringsList.Count).ToList();

                // Main filtering logic
                List<(int index, string str)> filteredWithIndices = new List<(int, string)>();

                if (wholeWords)
                {
                    string patternStr = @"\b" + Regex.Escape(searchStr) + @"\b";
                    RegexOptions options = caseSensitive ? RegexOptions.None : RegexOptions.IgnoreCase;
                    Regex regex = new Regex(patternStr, options);

                    for (int i = 0; i < stringsList.Count; i++)
                    {
                        if (regex.IsMatch(stringsList[i]))
                        {
                            filteredWithIndices.Add((i, stringsList[i]));
                        }
                    }
                }
                else
                {
                    if (caseSensitive)
                    {
                        for (int i = 0; i < stringsList.Count; i++)
                        {
                            if (stringsList[i].Contains(searchStr))
                            {
                                filteredWithIndices.Add((i, stringsList[i]));
                            }
                        }
                    }
                    else
                    {
                        string lowerSearchStr = searchStr.ToLower();
                        for (int i = 0; i < stringsList.Count; i++)
                        {
                            if (stringsList[i].ToLower().Contains(lowerSearchStr))
                            {
                                filteredWithIndices.Add((i, stringsList[i]));
                            }
                        }
                    }
                }

                // Extract results
                if (filteredWithIndices.Count > 0)
                {
                    filteredIndices = filteredWithIndices.Select(item => item.index).ToList();
                    filteredList = filteredWithIndices.Select(item => item.str).ToList();

                    // Update pattern
                    foreach (int idx in filteredIndices)
                    {
                        pattern[idx] = true;
                    }
                }

                count = filteredList.Count;

                // Create message
                string modeWholeWords = $"whole_words: {wholeWords}";
                string modeCaseSensitive = $"case_sensitive: {caseSensitive}";
                string matchCount = $"Number of matches: {count}";
                message = $"{modeWholeWords}\n{modeCaseSensitive}\n{matchCount}";
            }
            catch (Exception e)
            {
                message = $"Error: {e.Message}";
                filteredList.Clear();
                filteredIndices.Clear();
                count = 0;
                pattern.Clear();
            }

            // Set outputs
            DA.SetDataList(0, filteredList);
            DA.SetDataList(1, filteredIndices);
            DA.SetData(2, count);
            DA.SetDataList(3, pattern);

            // Update component message
            this.Message = message;
        }

        /// <summary>
        /// Provides an Icon for the component.
        /// </summary>
        protected override System.Drawing.Bitmap Icon
        {
            get
            {
                return null;
            }
        }

        /// <summary>
        /// Gets the unique ID for this component. Do not change this ID after release.
        /// </summary>
        public override Guid ComponentGuid
        {
            get { return new Guid("A3B11162-627B-455B-B9C0-963E76B36DC3"); }
        }
    }
}
