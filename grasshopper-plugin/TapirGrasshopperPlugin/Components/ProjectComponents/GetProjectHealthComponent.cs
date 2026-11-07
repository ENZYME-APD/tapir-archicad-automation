using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetProjectHealthComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetProjectHealth";

        // The items of the response, in the order of the outputs.
        private static readonly string[] HealthItems =
        {
            "unusedAttributes",
            "unusedLibraryParts",
            "missingLibraryParts",
            "brokenHotlinks",
            "outdatedHotlinks",
            "duplicateLayerNames",
            "emptyStories"
        };

        public GetProjectHealthComponent()
            : base(
                "GetProjectHealth",
                "A read-only summary of what a project cleanup looks at: the unused attributes (from Archicad 27) " +
                "and library parts, the missing library parts, the hotlinks whose source file is not available " +
                "(from Archicad 27), the layers whose names differ only in case or surrounding spaces, and the " +
                "stories that are the home story of no element. An item the Archicad API does not give is not " +
                "known (Known is false, with the reason), never guessed; whether a hotlink is outdated is never " +
                "known. Nothing is changed.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Items",
                "The name of each item: unusedAttributes, unusedLibraryParts, missingLibraryParts, " +
                "brokenHotlinks, outdatedHotlinks, duplicateLayerNames, emptyStories.");

            OutBooleans(
                "Known",
                "Whether each item is known. False when the Archicad API does not give it.");

            OutIntegers(
                "Counts",
                "The count of each item (for duplicateLayerNames the number of groups); null when it is not known.");

            OutBooleans(
                "Complete",
                "False when something of the item could not be read, so the count may be too high or too low; " +
                "null when the item has no such flag or is not known.");

            OutTexts(
                "Reasons",
                "Why each item is not known; empty when it is known.");

            OutTexts(
                "Details",
                "Each item as the command returned it, as JSON text (e.g. the counts per attribute type, the " +
                "names of the missing library parts, the broken hotlinks, the duplicate layers, the empty stories).");

            OutTexts(
                "Warnings",
                "The warnings of GetUnusedAttributes and GetUnusedLibraryParts, each starting with the command's name.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var items = new List<string>();
            var known = new List<object>();
            var counts = new List<object>();
            var complete = new List<object>();
            var reasons = new List<string>();
            var details = new List<string>();

            foreach (var itemName in HealthItems)
            {
                var item = response[itemName] as JObject;
                var isKnown = item?["known"]?.Type == JTokenType.Boolean && item["known"].Value<bool>();

                items.Add(itemName);
                known.Add(isKnown);
                counts.Add(isKnown ? JsonOutputHelp.Scalar(item, "count") : null);
                complete.Add(isKnown ? JsonOutputHelp.Scalar(item, "complete") : null);
                reasons.Add(isKnown ? "" : item?["reason"]?.ToString() ?? "");
                details.Add(item?.ToString(Formatting.Indented));
            }

            var warnings = new List<string>();
            foreach (var warning in JsonOutputHelp.Items(response, "warnings"))
            {
                warnings.Add(warning?.ToString());
            }

            da.SetDataList(0, items);
            da.SetDataList(1, known);
            da.SetDataList(2, counts);
            da.SetDataList(3, complete);
            da.SetDataList(4, reasons);
            da.SetDataList(5, details);
            da.SetDataList(6, warnings);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetProjectInfoGroups;

        public override Guid ComponentGuid =>
            new Guid("e1c53f8a-6d24-4b97-9f0a-73b8d2e46c15");
    }
}
