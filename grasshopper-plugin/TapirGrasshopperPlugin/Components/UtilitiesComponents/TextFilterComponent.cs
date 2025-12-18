using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using TapirGrasshopperPlugin.Helps;

// ReSharper disable All

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class TextFilterComponent : Component
    {
        public TextFilterComponent()
            : base(
                "TextFilter",
                "Filters a list of strings based on a search string.",
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Strings",
                "A list of strings to search through.");

            InTexts(
                "FilterStrings",
                "The strings to search for within the Strings input list.");

            InBoolean(
                "CaseSensitive",
                "Determines if the search is case-sensitive.",
                false);

            InBoolean(
                "WholeWords",
                "Determines if the search matches whole words only.",
                true);

            SetOptionality(
                new[]
                {
                    2,
                    3
                });
        }

        protected override void AddOutputs()
        {
            OutTextTree(
                "FilteredTexts",
                "List of strings matching the search filter.");

            OutIntegerTree(
                "FilteredIndices",
                "List of indices of the matched strings in the original list.");

            OutBooleanTree(
                "MatchPatterns",
                "Booleans indicating whether a string matched or not.");
        }

        private Regex BuildRegex(
            string filter,
            bool isWholeWords,
            bool isCaseSensitive)
        {
            var escape = Regex.Escape(filter);
            var pattern = isWholeWords ? $@"\b{escape}\b" : escape;

            RegexOptions options = isCaseSensitive
                ? RegexOptions.None
                : RegexOptions.IgnoreCase;

            return new Regex(
                pattern,
                options);
        }

        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> textsToFilter))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> filters))
            {
                return;
            }

            var isCaseSensitive = da.GetOptional(
                2,
                false);

            var isWholeWords = da.GetOptional(
                3,
                true);

            var treeFiltered = new DataTree<string>();
            var treeIndices = new DataTree<int>();
            var treeMask = new DataTree<bool>();

            for (int i = 0; i < filters.Count; i++)
            {
                var filter = filters[i];
                var path = new GH_Path(i);
                var regex = BuildRegex(
                    filter,
                    isWholeWords,
                    isCaseSensitive);

                for (int j = 0; j < textsToFilter.Count; j++)
                {
                    var text = textsToFilter[j];
                    var isMatch = regex.IsMatch(text);

                    treeMask.Add(
                        isMatch,
                        path);

                    if (isMatch)
                    {
                        treeFiltered.Add(
                            text,
                            path);

                        treeIndices.Add(
                            j,
                            path);
                    }
                }
            }

            da.SetDataTree(
                0,
                treeFiltered);

            da.SetDataTree(
                1,
                treeIndices);

            da.SetDataTree(
                2,
                treeMask);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.FilterContainsString;

        public override Guid ComponentGuid =>
            new Guid("A3B11162-627B-455B-B9C0-963E76B36DC3");
    }
}