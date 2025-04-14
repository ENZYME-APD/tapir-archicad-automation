using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Linq;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class ParalellListFilterComponent : Component
    {
        public ParalellListFilterComponent ()
            : base ("Parallel List Filter", "PLFilter",
                  "Filters two parallel lists based on values in the primary list", "Utilities")
        {
        }

        protected override void RegisterInputParams (GH_Component.GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("Primary List", "P", "Primary list for filtering", GH_ParamAccess.list);
            pManager.AddGenericParameter ("Secondary List", "S", "Secondary list that gets filtered in parallel", GH_ParamAccess.list);
            pManager.AddGenericParameter ("Search Values", "V", "Search values for filtering", GH_ParamAccess.list);
            pManager.AddTextParameter ("Filter Mode", "M", "Filter mode (equals, notFound, greater, smaller, range, closest, group_equals, group_unique)", GH_ParamAccess.item, "equals");
        }

        protected override void RegisterOutputParams (GH_Component.GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("Filtered Primary", "FP", "Filtered primary list", GH_ParamAccess.tree);
            pManager.AddGenericParameter ("Filtered Secondary", "FS", "Filtered secondary list", GH_ParamAccess.tree);
            pManager.AddBooleanParameter ("Filter Pattern", "P", "Boolean pattern indicating which items were filtered", GH_ParamAccess.list);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            // Get inputs
            List<IGH_Goo> primaryList = new List<IGH_Goo> ();
            if (!DA.GetDataList (0, primaryList)) {
                this.Message = "Waiting for all inputs to be provided.";
                return;
            }

            List<IGH_Goo> secondaryList = new List<IGH_Goo> ();
            if (!DA.GetDataList (1, secondaryList)) {
                this.Message = "Waiting for all inputs to be provided.";
                return;
            }

            List<IGH_Goo> searchValues = new List<IGH_Goo> ();
            if (!DA.GetDataList (2, searchValues)) {
                this.Message = "Waiting for all inputs to be provided.";
                return;
            }

            string filterMode = "equals";
            DA.GetData (3, ref filterMode);

            // Initialize outputs
            GH_Structure<IGH_Goo> filteredPrimary = new GH_Structure<IGH_Goo> ();
            GH_Structure<IGH_Goo> filteredSecondary = new GH_Structure<IGH_Goo> ();
            List<bool> pattern = new List<bool> ();
            string message = "Waiting for all inputs to be provided.";

            // Filter the lists
            var result = FilterLists (primaryList, secondaryList, searchValues, filterMode);
            filteredPrimary = result.filteredPrimary;
            filteredSecondary = result.filteredSecondary;
            pattern = result.pattern;
            message = result.message;

            // Set outputs
            DA.SetDataTree (0, filteredPrimary);
            DA.SetDataTree (1, filteredSecondary);
            DA.SetDataList (2, pattern);

            // Update component message
            this.Message = message;
        }

        private (GH_Structure<IGH_Goo> filteredPrimary, GH_Structure<IGH_Goo> filteredSecondary, List<bool> pattern, string message)
            FilterLists (List<IGH_Goo> primaryList, List<IGH_Goo> secondaryList, List<IGH_Goo> searchValues, string filterMode)
        {
            // Initialize output structures
            GH_Structure<IGH_Goo> filteredPrimary = new GH_Structure<IGH_Goo> ();
            GH_Structure<IGH_Goo> filteredSecondary = new GH_Structure<IGH_Goo> ();
            List<bool> pattern = new List<bool> ();
            string message = "";

            if (primaryList != null && secondaryList != null && !string.IsNullOrEmpty (filterMode)) {
                // Check if lists have matching lengths
                if (primaryList.Count != secondaryList.Count) {
                    message = $"Error: Lists must have equal length.\nprimList: {primaryList.Count}, secList: {secondaryList.Count}";
                    return (filteredPrimary, filteredSecondary, pattern, message);
                }

                pattern = Enumerable.Repeat (false, primaryList.Count).ToList ();  // Initialize pattern with False

                // Convert search values to comparable types
                List<double> numericSearchValues = new List<double> ();
                foreach (var val in searchValues) {
                    if (GH_Convert.ToDouble (val, out double dVal, GH_Conversion.Both)) {
                        numericSearchValues.Add (dVal);
                    }
                }

                if (filterMode == "equals") {
                    List<IGH_Goo> filteredPrimaryList = new List<IGH_Goo> ();
                    List<IGH_Goo> filteredSecondaryList = new List<IGH_Goo> ();

                    for (int i = 0; i < primaryList.Count; i++) {
                        if (searchValues.Any (sv => GH_Convert.ToDouble (sv, out double svVal, GH_Conversion.Both) &&
                                                  GH_Convert.ToDouble (primaryList[i], out double pVal, GH_Conversion.Both) &&
                                                  Math.Abs (svVal - pVal) < 1e-10)) {
                            filteredPrimaryList.Add (primaryList[i]);
                            filteredSecondaryList.Add (secondaryList[i]);
                            pattern[i] = true;
                        }
                    }

                    filteredPrimary.AppendRange (filteredPrimaryList, new GH_Path (0));
                    filteredSecondary.AppendRange (filteredSecondaryList, new GH_Path (0));
                    message = $"Equals {string.Join (", ", searchValues)}\nFound {filteredPrimaryList.Count}/{primaryList.Count}";
                } else if (filterMode == "notFound") {
                    List<IGH_Goo> filteredPrimaryList = new List<IGH_Goo> ();
                    List<IGH_Goo> filteredSecondaryList = new List<IGH_Goo> ();

                    for (int i = 0; i < primaryList.Count; i++) {
                        if (!searchValues.Any (sv => GH_Convert.ToDouble (sv, out double svVal, GH_Conversion.Both) &&
                                                   GH_Convert.ToDouble (primaryList[i], out double pVal, GH_Conversion.Both) &&
                                                   Math.Abs (svVal - pVal) < 1e-10)) {
                            filteredPrimaryList.Add (primaryList[i]);
                            filteredSecondaryList.Add (secondaryList[i]);
                            pattern[i] = true;
                        }
                    }

                    filteredPrimary.AppendRange (filteredPrimaryList, new GH_Path (0));
                    filteredSecondary.AppendRange (filteredSecondaryList, new GH_Path (0));
                    message = $"Not Found in 'sValues' \n Not found {filteredPrimaryList.Count}/{primaryList.Count}";
                } else if (filterMode == "greater" && numericSearchValues.Count > 0) {
                    List<IGH_Goo> filteredPrimaryList = new List<IGH_Goo> ();
                    List<IGH_Goo> filteredSecondaryList = new List<IGH_Goo> ();
                    double maxSearchValue = numericSearchValues.Max ();

                    for (int i = 0; i < primaryList.Count; i++) {
                        if (GH_Convert.ToDouble (primaryList[i], out double pVal, GH_Conversion.Both) && pVal > maxSearchValue) {
                            filteredPrimaryList.Add (primaryList[i]);
                            filteredSecondaryList.Add (secondaryList[i]);
                            pattern[i] = true;
                        }
                    }

                    filteredPrimary.AppendRange (filteredPrimaryList, new GH_Path (0));
                    filteredSecondary.AppendRange (filteredSecondaryList, new GH_Path (0));
                    message = $"Greater than {maxSearchValue}\nFound {filteredPrimaryList.Count}/{primaryList.Count}";
                } else if (filterMode == "smaller" && numericSearchValues.Count > 0) {
                    List<IGH_Goo> filteredPrimaryList = new List<IGH_Goo> ();
                    List<IGH_Goo> filteredSecondaryList = new List<IGH_Goo> ();
                    double minSearchValue = numericSearchValues.Min ();

                    for (int i = 0; i < primaryList.Count; i++) {
                        if (GH_Convert.ToDouble (primaryList[i], out double pVal, GH_Conversion.Both) && pVal < minSearchValue) {
                            filteredPrimaryList.Add (primaryList[i]);
                            filteredSecondaryList.Add (secondaryList[i]);
                            pattern[i] = true;
                        }
                    }

                    filteredPrimary.AppendRange (filteredPrimaryList, new GH_Path (0));
                    filteredSecondary.AppendRange (filteredSecondaryList, new GH_Path (0));
                    message = $"Smaller than {minSearchValue}\nFound {filteredPrimaryList.Count}/{primaryList.Count}";
                } else if (filterMode == "range" && numericSearchValues.Count >= 2) {
                    List<IGH_Goo> filteredPrimaryList = new List<IGH_Goo> ();
                    List<IGH_Goo> filteredSecondaryList = new List<IGH_Goo> ();
                    double minVal = numericSearchValues.Min ();
                    double maxVal = numericSearchValues.Max ();

                    for (int i = 0; i < primaryList.Count; i++) {
                        if (GH_Convert.ToDouble (primaryList[i], out double pVal, GH_Conversion.Both) && pVal >= minVal && pVal <= maxVal) {
                            filteredPrimaryList.Add (primaryList[i]);
                            filteredSecondaryList.Add (secondaryList[i]);
                            pattern[i] = true;
                        }
                    }

                    filteredPrimary.AppendRange (filteredPrimaryList, new GH_Path (0));
                    filteredSecondary.AppendRange (filteredSecondaryList, new GH_Path (0));
                    message = $"Range {minVal} to {maxVal}\nFound {filteredPrimaryList.Count}/{primaryList.Count}";
                } else if (filterMode == "closest" && numericSearchValues.Count > 0) {
                    List<IGH_Goo> filteredPrimaryList = new List<IGH_Goo> ();
                    List<IGH_Goo> filteredSecondaryList = new List<IGH_Goo> ();
                    HashSet<double> mappedValues = new HashSet<double> ();

                    for (int i = 0; i < primaryList.Count; i++) {
                        if (GH_Convert.ToDouble (primaryList[i], out double pVal, GH_Conversion.Both)) {
                            double closestVal = numericSearchValues.OrderBy (sv => Math.Abs (sv - pVal)).First ();

                            // Create a new GH_Number with the closest value
                            GH_Number closestGhNumber = new GH_Number (closestVal);

                            filteredPrimaryList.Add (closestGhNumber);
                            filteredSecondaryList.Add (secondaryList[i]);
                            pattern[i] = true;
                            mappedValues.Add (closestVal);
                        }
                    }

                    filteredPrimary.AppendRange (filteredPrimaryList, new GH_Path (0));
                    filteredSecondary.AppendRange (filteredSecondaryList, new GH_Path (0));
                    message = $"Closest to {string.Join (", ", numericSearchValues)}\nMapped to {mappedValues.Count} values";
                } else if (filterMode == "group_equals") {
                    Dictionary<double, List<int>> groups = new Dictionary<double, List<int>> ();

                    // Initialize groups for each search value
                    foreach (var val in searchValues) {
                        if (GH_Convert.ToDouble (val, out double dVal, GH_Conversion.Both)) {
                            groups[dVal] = new List<int> ();
                        }
                    }

                    // Group indices by search values
                    for (int i = 0; i < primaryList.Count; i++) {
                        if (GH_Convert.ToDouble (primaryList[i], out double pVal, GH_Conversion.Both)) {
                            foreach (var kvp in groups) {
                                if (Math.Abs (kvp.Key - pVal) < 1e-10) {
                                    kvp.Value.Add (i);
                                    pattern[i] = true;
                                    break;
                                }
                            }
                        }
                    }

                    // Create tree branches for each group
                    int totalMatches = 0;
                    int idx = 0;
                    foreach (var kvp in groups) {
                        GH_Path path = new GH_Path (idx);
                        List<IGH_Goo> primValues = kvp.Value.Select (i => primaryList[i]).ToList ();
                        List<IGH_Goo> secValues = kvp.Value.Select (i => secondaryList[i]).ToList ();

                        if (primValues.Count > 0) {
                            filteredPrimary.AppendRange (primValues, path);
                            filteredSecondary.AppendRange (secValues, path);
                            totalMatches += primValues.Count;
                        }

                        idx++;
                    }

                    message = $"Group Equals {string.Join (", ", searchValues)}\nFound {totalMatches} in {groups.Count} branches";
                } else if (filterMode == "group_unique") {
                    // Get unique values from secondary list
                    HashSet<string> uniqueValues = new HashSet<string> ();
                    Dictionary<string, List<int>> groups = new Dictionary<string, List<int>> ();

                    // First pass: identify unique values
                    for (int i = 0; i < secondaryList.Count; i++) {
                        string secValue = secondaryList[i].ToString ();
                        uniqueValues.Add (secValue);
                    }

                    // Initialize groups for each unique value
                    foreach (var val in uniqueValues) {
                        groups[val] = new List<int> ();
                    }

                    // Second pass: group indices by unique values
                    for (int i = 0; i < secondaryList.Count; i++) {
                        string secValue = secondaryList[i].ToString ();
                        groups[secValue].Add (i);
                        pattern[i] = true;  // All items are grouped
                    }

                    // Create tree branches for each group
                    int totalMatches = 0;
                    int idx = 0;
                    foreach (var kvp in groups) {
                        GH_Path path = new GH_Path (idx);
                        List<IGH_Goo> primValues = kvp.Value.Select (i => primaryList[i]).ToList ();
                        List<IGH_Goo> secValues = kvp.Value.Select (i => secondaryList[i]).ToList ();

                        if (primValues.Count > 0) {
                            filteredPrimary.AppendRange (primValues, path);
                            filteredSecondary.AppendRange (secValues, path);
                            totalMatches += primValues.Count;
                        }

                        idx++;
                    }

                    message = $"Group Unique from secList\nFound {totalMatches} in {uniqueValues.Count} branches";
                } else {
                    message = $"Invalid filter mode. Use 'equals', 'notFound', 'greater', 'smaller', 'range', 'closest', 'group_equals', or 'group_unique'.";
                }
            } else {
                message = "Waiting for all inputs to be provided.";
            }

            return (filteredPrimary, filteredSecondary, pattern, message);
        }

        protected override System.Drawing.Bitmap Icon => Properties.Resources.GH_ListItemVar;

        public override Guid ComponentGuid => new Guid ("A5B11162-627B-455B-B9C0-963E76B36DC5");
    }
}
