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
        public static string Doc =>
            "Filters two parallel lists based on values in the primary list";

        public ParalellListFilterComponent()
            : base(
                "Parallel List Filter",
                "PLFilter",
                Doc,
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "Primary List",
                "Primary list for filtering");

            InGenerics(
                "Secondary List",
                "Secondary list that gets filtered in parallel");

            InGenerics(
                "Search Values",
                "Search values for filtering");

            InText(
                "Filter Mode",
                "Filter mode (equals, notFound, greater, smaller, range, closest, group_equals, group_unique)",
                "equals");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "Filtered Primary",
                "Filtered primary list");

            OutGenericTree(
                "Filtered Secondary",
                "Filtered secondary list");

            OutBooleans(
                "Filter Pattern",
                "Boolean pattern indicating which items were filtered");
        }

        protected override void SolveInstance(
            IGH_DataAccess DA)
        {
            // Get inputs
            var primaryList = new List<IGH_Goo>();
            if (!DA.GetDataList(
                    0,
                    primaryList))
            {
                Message = "Waiting for all inputs to be provided.";
                return;
            }

            var secondaryList = new List<IGH_Goo>();
            if (!DA.GetDataList(
                    1,
                    secondaryList))
            {
                Message = "Waiting for all inputs to be provided.";
                return;
            }

            var searchValues = new List<IGH_Goo>();
            if (!DA.GetDataList(
                    2,
                    searchValues))
            {
                Message = "Waiting for all inputs to be provided.";
                return;
            }

            var filterMode = "equals";
            DA.GetData(
                3,
                ref filterMode);

            // Initialize outputs
            var filteredPrimary = new GH_Structure<IGH_Goo>();
            var filteredSecondary = new GH_Structure<IGH_Goo>();
            var pattern = new List<bool>();
            var message = "Waiting for all inputs to be provided.";

            // Filter the lists
            var result = FilterLists(
                primaryList,
                secondaryList,
                searchValues,
                filterMode);
            filteredPrimary = result.filteredPrimary;
            filteredSecondary = result.filteredSecondary;
            pattern = result.pattern;
            message = result.message;

            // Set outputs
            DA.SetDataTree(
                0,
                filteredPrimary);
            DA.SetDataTree(
                1,
                filteredSecondary);
            DA.SetDataList(
                2,
                pattern);

            // Update component message
            Message = message;
        }

        private (GH_Structure<IGH_Goo> filteredPrimary, GH_Structure<IGH_Goo>
            filteredSecondary, List<bool> pattern, string message) FilterLists(
                List<IGH_Goo> primaryList,
                List<IGH_Goo> secondaryList,
                List<IGH_Goo> searchValues,
                string filterMode)
        {
            // Initialize output structures
            var filteredPrimary = new GH_Structure<IGH_Goo>();
            var filteredSecondary = new GH_Structure<IGH_Goo>();
            var pattern = new List<bool>();
            var message = "";

            if (primaryList != null && secondaryList != null &&
                !string.IsNullOrEmpty(filterMode))
            {
                // Check if lists have matching lengths
                if (primaryList.Count != secondaryList.Count)
                {
                    message =
                        $"Error: Lists must have equal length.\nprimList: {primaryList.Count}, secList: {secondaryList.Count}";
                    return (filteredPrimary, filteredSecondary, pattern,
                        message);
                }

                pattern = Enumerable
                    .Repeat(
                        false,
                        primaryList.Count)
                    .ToList(); // Initialize pattern with False

                // Convert search values to comparable types
                var numericSearchValues = new List<double>();
                foreach (var val in searchValues)
                {
                    if (GH_Convert.ToDouble(
                            val,
                            out var dVal,
                            GH_Conversion.Both))
                    {
                        numericSearchValues.Add(dVal);
                    }
                }

                if (filterMode == "equals")
                {
                    var filteredPrimaryList = new List<IGH_Goo>();
                    var filteredSecondaryList = new List<IGH_Goo>();

                    for (var i = 0; i < primaryList.Count; i++)
                    {
                        if (searchValues.Any(sv => GH_Convert.ToDouble(
                                                       sv,
                                                       out var svVal,
                                                       GH_Conversion.Both) &&
                                                   GH_Convert.ToDouble(
                                                       primaryList[i],
                                                       out var pVal,
                                                       GH_Conversion.Both) &&
                                                   Math.Abs(svVal - pVal) <
                                                   1e-10))
                        {
                            filteredPrimaryList.Add(primaryList[i]);
                            filteredSecondaryList.Add(secondaryList[i]);
                            pattern[i] = true;
                        }
                    }

                    filteredPrimary.AppendRange(
                        filteredPrimaryList,
                        new GH_Path(0));
                    filteredSecondary.AppendRange(
                        filteredSecondaryList,
                        new GH_Path(0));
                    message =
                        $"Equals {string.Join(", ", searchValues)}\nFound {filteredPrimaryList.Count}/{primaryList.Count}";
                }
                else if (filterMode == "notFound")
                {
                    var filteredPrimaryList = new List<IGH_Goo>();
                    var filteredSecondaryList = new List<IGH_Goo>();

                    for (var i = 0; i < primaryList.Count; i++)
                    {
                        if (!searchValues.Any(sv => GH_Convert.ToDouble(
                                                        sv,
                                                        out var svVal,
                                                        GH_Conversion.Both) &&
                                                    GH_Convert.ToDouble(
                                                        primaryList[i],
                                                        out var pVal,
                                                        GH_Conversion.Both) &&
                                                    Math.Abs(svVal - pVal) <
                                                    1e-10))
                        {
                            filteredPrimaryList.Add(primaryList[i]);
                            filteredSecondaryList.Add(secondaryList[i]);
                            pattern[i] = true;
                        }
                    }

                    filteredPrimary.AppendRange(
                        filteredPrimaryList,
                        new GH_Path(0));
                    filteredSecondary.AppendRange(
                        filteredSecondaryList,
                        new GH_Path(0));
                    message =
                        $"Not Found in 'sValues' \n Not found {filteredPrimaryList.Count}/{primaryList.Count}";
                }
                else if (filterMode == "greater" &&
                         numericSearchValues.Count > 0)
                {
                    var filteredPrimaryList = new List<IGH_Goo>();
                    var filteredSecondaryList = new List<IGH_Goo>();
                    var maxSearchValue = numericSearchValues.Max();

                    for (var i = 0; i < primaryList.Count; i++)
                    {
                        if (GH_Convert.ToDouble(
                                primaryList[i],
                                out var pVal,
                                GH_Conversion.Both) && pVal > maxSearchValue)
                        {
                            filteredPrimaryList.Add(primaryList[i]);
                            filteredSecondaryList.Add(secondaryList[i]);
                            pattern[i] = true;
                        }
                    }

                    filteredPrimary.AppendRange(
                        filteredPrimaryList,
                        new GH_Path(0));
                    filteredSecondary.AppendRange(
                        filteredSecondaryList,
                        new GH_Path(0));
                    message =
                        $"Greater than {maxSearchValue}\nFound {filteredPrimaryList.Count}/{primaryList.Count}";
                }
                else if (filterMode == "smaller" &&
                         numericSearchValues.Count > 0)
                {
                    var filteredPrimaryList = new List<IGH_Goo>();
                    var filteredSecondaryList = new List<IGH_Goo>();
                    var minSearchValue = numericSearchValues.Min();

                    for (var i = 0; i < primaryList.Count; i++)
                    {
                        if (GH_Convert.ToDouble(
                                primaryList[i],
                                out var pVal,
                                GH_Conversion.Both) && pVal < minSearchValue)
                        {
                            filteredPrimaryList.Add(primaryList[i]);
                            filteredSecondaryList.Add(secondaryList[i]);
                            pattern[i] = true;
                        }
                    }

                    filteredPrimary.AppendRange(
                        filteredPrimaryList,
                        new GH_Path(0));
                    filteredSecondary.AppendRange(
                        filteredSecondaryList,
                        new GH_Path(0));
                    message =
                        $"Smaller than {minSearchValue}\nFound {filteredPrimaryList.Count}/{primaryList.Count}";
                }
                else if (filterMode == "range" &&
                         numericSearchValues.Count >= 2)
                {
                    var filteredPrimaryList = new List<IGH_Goo>();
                    var filteredSecondaryList = new List<IGH_Goo>();
                    var minVal = numericSearchValues.Min();
                    var maxVal = numericSearchValues.Max();

                    for (var i = 0; i < primaryList.Count; i++)
                    {
                        if (GH_Convert.ToDouble(
                                primaryList[i],
                                out var pVal,
                                GH_Conversion.Both) && pVal >= minVal &&
                            pVal <= maxVal)
                        {
                            filteredPrimaryList.Add(primaryList[i]);
                            filteredSecondaryList.Add(secondaryList[i]);
                            pattern[i] = true;
                        }
                    }

                    filteredPrimary.AppendRange(
                        filteredPrimaryList,
                        new GH_Path(0));
                    filteredSecondary.AppendRange(
                        filteredSecondaryList,
                        new GH_Path(0));
                    message =
                        $"Range {minVal} to {maxVal}\nFound {filteredPrimaryList.Count}/{primaryList.Count}";
                }
                else if (filterMode == "closest" &&
                         numericSearchValues.Count > 0)
                {
                    var filteredPrimaryList = new List<IGH_Goo>();
                    var filteredSecondaryList = new List<IGH_Goo>();
                    var mappedValues = new HashSet<double>();

                    for (var i = 0; i < primaryList.Count; i++)
                    {
                        if (GH_Convert.ToDouble(
                                primaryList[i],
                                out var pVal,
                                GH_Conversion.Both))
                        {
                            var closestVal = numericSearchValues
                                .OrderBy(sv => Math.Abs(sv - pVal))
                                .First();

                            // Create a new GH_Number with the closest value
                            var closestGhNumber = new GH_Number(closestVal);

                            filteredPrimaryList.Add(closestGhNumber);
                            filteredSecondaryList.Add(secondaryList[i]);
                            pattern[i] = true;
                            mappedValues.Add(closestVal);
                        }
                    }

                    filteredPrimary.AppendRange(
                        filteredPrimaryList,
                        new GH_Path(0));
                    filteredSecondary.AppendRange(
                        filteredSecondaryList,
                        new GH_Path(0));
                    message =
                        $"Closest to {string.Join(", ", numericSearchValues)}\nMapped to {mappedValues.Count} values";
                }
                else if (filterMode == "group_equals")
                {
                    var groups = new Dictionary<double, List<int>>();

                    // Initialize groups for each search value
                    foreach (var val in searchValues)
                    {
                        if (GH_Convert.ToDouble(
                                val,
                                out var dVal,
                                GH_Conversion.Both))
                        {
                            groups[dVal] = new List<int>();
                        }
                    }

                    // Group indices by search values
                    for (var i = 0; i < primaryList.Count; i++)
                    {
                        if (GH_Convert.ToDouble(
                                primaryList[i],
                                out var pVal,
                                GH_Conversion.Both))
                        {
                            foreach (var kvp in groups)
                            {
                                if (Math.Abs(kvp.Key - pVal) < 1e-10)
                                {
                                    kvp.Value.Add(i);
                                    pattern[i] = true;
                                    break;
                                }
                            }
                        }
                    }

                    // Create tree branches for each group
                    var totalMatches = 0;
                    var idx = 0;
                    foreach (var kvp in groups)
                    {
                        var path = new GH_Path(idx);
                        var primValues =
                            kvp.Value.Select(i => primaryList[i]).ToList();
                        var secValues = kvp
                            .Value.Select(i => secondaryList[i])
                            .ToList();

                        if (primValues.Count > 0)
                        {
                            filteredPrimary.AppendRange(
                                primValues,
                                path);
                            filteredSecondary.AppendRange(
                                secValues,
                                path);
                            totalMatches += primValues.Count;
                        }

                        idx++;
                    }

                    message =
                        $"Group Equals {string.Join(", ", searchValues)}\nFound {totalMatches} in {groups.Count} branches";
                }
                else if (filterMode == "group_unique")
                {
                    // Get unique values from secondary list
                    var uniqueValues = new HashSet<string>();
                    var groups = new Dictionary<string, List<int>>();

                    // First pass: identify unique values
                    for (var i = 0; i < secondaryList.Count; i++)
                    {
                        var secValue = secondaryList[i].ToString();
                        uniqueValues.Add(secValue);
                    }

                    // Initialize groups for each unique value
                    foreach (var val in uniqueValues)
                    {
                        groups[val] = new List<int>();
                    }

                    // Second pass: group indices by unique values
                    for (var i = 0; i < secondaryList.Count; i++)
                    {
                        var secValue = secondaryList[i].ToString();
                        groups[secValue].Add(i);
                        pattern[i] = true; // All items are grouped
                    }

                    // Create tree branches for each group
                    var totalMatches = 0;
                    var idx = 0;
                    foreach (var kvp in groups)
                    {
                        var path = new GH_Path(idx);
                        var primValues =
                            kvp.Value.Select(i => primaryList[i]).ToList();
                        var secValues = kvp
                            .Value.Select(i => secondaryList[i])
                            .ToList();

                        if (primValues.Count > 0)
                        {
                            filteredPrimary.AppendRange(
                                primValues,
                                path);
                            filteredSecondary.AppendRange(
                                secValues,
                                path);
                            totalMatches += primValues.Count;
                        }

                        idx++;
                    }

                    message =
                        $"Group Unique from secList\nFound {totalMatches} in {uniqueValues.Count} branches";
                }
                else
                {
                    message =
                        $"Invalid filter mode. Use 'equals', 'notFound', 'greater', 'smaller', 'range', 'closest', 'group_equals', or 'group_unique'.";
                }
            }
            else
            {
                message = "Waiting for all inputs to be provided.";
            }

            return (filteredPrimary, filteredSecondary, pattern, message);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ParallelListFilter;

        public override Guid ComponentGuid =>
            new Guid("A5B11162-627B-455B-B9C0-963E76B36DC5");
    }
}