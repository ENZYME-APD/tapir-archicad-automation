using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class TextFilterComponent : Component
    {
        public static string Doc =>
            "Filters a list of strings based on a search string.";

        public TextFilterComponent()
            : base(
                "Filter Contains String",
                "Filter Contains",
                Doc,
                GroupNames.Utilities)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Strings List",
                "L",
                "A list of strings to search through",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "Search String",
                "S",
                "The string to search for within the strings list",
                GH_ParamAccess.item);
            pManager.AddBooleanParameter(
                "Case Sensitive",
                "C",
                "Determines if the search is case-sensitive",
                GH_ParamAccess.item,
                false);
            pManager[2].Optional = true;
            pManager.AddBooleanParameter(
                "Whole Words",
                "W",
                "Determines if the search matches whole words only",
                GH_ParamAccess.item,
                true);
            pManager[3].Optional = true;
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Filtered List",
                "F",
                "List of strings that match the search criteria",
                GH_ParamAccess.list);
            pManager.AddIntegerParameter(
                "Filtered Indices",
                "I",
                "List of indices of the matched strings",
                GH_ParamAccess.list);
            pManager.AddIntegerParameter(
                "Count",
                "C",
                "Number of matches",
                GH_ParamAccess.item);
            pManager.AddBooleanParameter(
                "Pattern",
                "P",
                "Boolean list indicating which strings matched",
                GH_ParamAccess.list);
        }

        protected override void SolveInstance(
            IGH_DataAccess DA)
        {
            // Initialize outputs
            var filteredList = new List<string>();
            var filteredIndices = new List<int>();
            var count = 0;
            var pattern = new List<bool>();
            var message = "Waiting for input strings.";

            // Get inputs
            var stringsList = new List<string>();
            if (!DA.GetDataList(
                    0,
                    stringsList) || stringsList.Count == 0)
            {
                message =
                    "Error: Provide both a strings list and a search string.";
                Message = message;
                DA.SetDataList(
                    0,
                    filteredList);
                DA.SetDataList(
                    1,
                    filteredIndices);
                DA.SetData(
                    2,
                    count);
                DA.SetDataList(
                    3,
                    pattern);
                return;
            }

            var searchStr = "";
            if (!DA.GetData(
                    1,
                    ref searchStr) || string.IsNullOrEmpty(searchStr))
            {
                message =
                    "Error: Provide both a strings list and a search string.";
                Message = message;
                DA.SetDataList(
                    0,
                    filteredList);
                DA.SetDataList(
                    1,
                    filteredIndices);
                DA.SetData(
                    2,
                    count);
                DA.SetDataList(
                    3,
                    pattern);
                return;
            }

            var caseSensitive = false;
            DA.GetData(
                2,
                ref caseSensitive);

            var wholeWords = true;
            DA.GetData(
                3,
                ref wholeWords);

            try
            {
                // Initialize pattern with false for all items
                pattern = Enumerable
                    .Repeat(
                        false,
                        stringsList.Count)
                    .ToList();

                // Main filtering logic
                var filteredWithIndices = new List<(int, string)>();

                if (wholeWords)
                {
                    var patternStr = @"\b" + Regex.Escape(searchStr) + @"\b";
                    var options = caseSensitive
                        ? RegexOptions.None
                        : RegexOptions.IgnoreCase;
                    var regex = new Regex(
                        patternStr,
                        options);

                    for (var i = 0; i < stringsList.Count; i++)
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
                        for (var i = 0; i < stringsList.Count; i++)
                        {
                            if (stringsList[i].Contains(searchStr))
                            {
                                filteredWithIndices.Add((i, stringsList[i]));
                            }
                        }
                    }
                    else
                    {
                        var lowerSearchStr = searchStr.ToLower();
                        for (var i = 0; i < stringsList.Count; i++)
                        {
                            if (stringsList[i]
                                .ToLower()
                                .Contains(lowerSearchStr))
                            {
                                filteredWithIndices.Add((i, stringsList[i]));
                            }
                        }
                    }
                }

                // Extract results
                if (filteredWithIndices.Count > 0)
                {
                    filteredIndices = filteredWithIndices
                        .Select(item => item.Item1)
                        .ToList();
                    filteredList = filteredWithIndices
                        .Select(item => item.Item2)
                        .ToList();

                    // Update pattern
                    foreach (var idx in filteredIndices)
                    {
                        pattern[idx] = true;
                    }
                }

                count = filteredList.Count;

                // Create message
                var modeWholeWords = $"whole_words: {wholeWords}";
                var modeCaseSensitive = $"case_sensitive: {caseSensitive}";
                var matchCount = $"Number of matches: {count}";
                message =
                    $"{modeWholeWords}\n{modeCaseSensitive}\n{matchCount}";
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
            DA.SetDataList(
                0,
                filteredList);
            DA.SetDataList(
                1,
                filteredIndices);
            DA.SetData(
                2,
                count);
            DA.SetDataList(
                3,
                pattern);

            // Update component message
            Message = message;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.FilterContainsString;

        public override Guid ComponentGuid =>
            new Guid("A3B11162-627B-455B-B9C0-963E76B36DC3");
    }
}