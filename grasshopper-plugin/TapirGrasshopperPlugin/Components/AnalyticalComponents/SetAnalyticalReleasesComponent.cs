using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class SetAnalyticalReleasesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetAnalyticalReleases";

        public SetAnalyticalReleasesComponent()
            : base(
                "SetAnalyticalReleases",
                "Replaces the releases of the given analytical members, in one undo step. " +
                "Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements of the analytical members.");

            InTexts(
                "Releases",
                "The new release of each element as JSON, as GetAnalyticalReleases returns it " +
                "(input only 1 to use the same release for all). It replaces the release as a whole.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when its release was set).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var elementParameters = new JObject();
            if (!AnalyticalJsonHelp.TryAddElements(da, 0, elementParameters) ||
                !da.TryGetList(1, out List<string> texts))
            {
                return;
            }
            var elements = (JArray)elementParameters["elements"];

            if (!AnalyticalHelp.CheckRequiredCount(this, texts.Count, elements.Count, "Releases", "ElementGuids") ||
                !AnalyticalJsonHelp.TryParseObjects(this, texts, "Releases", out List<JObject> releases))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < elements.Count; i++)
            {
                items.Add(
                    new JObject
                    {
                        ["elementId"] = elements[i]["elementId"],
                        ["release"] = AnalyticalJsonHelp.Unwrap(
                            AnalyticalHelp.ItemFor(releases, i),
                            "release")
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["releases"] = items },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetAnalyticalVisibilitySettings;

        public override Guid ComponentGuid =>
            new Guid("d8b59c53-f8b0-4e4b-b179-c84885785879");
    }
}
