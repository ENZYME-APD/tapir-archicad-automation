using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class ConvertAnalyticalLinksToNonShortestComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ConvertAnalyticalLinksToNonShortest";

        public ConvertAnalyticalLinksToNonShortestComponent()
            : base(
                "ConvertAnalyticalLinksToNonShortest",
                "Converts the given shortest analytical links (associative to a curve member or a surface member " +
                "edge) to links connected to a fixed coordinate, in one undo step, and returns the converted " +
                "links. Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the shortest analytical links to convert.");

            InGeneric(
                "AnalyticalModelGuid",
                AnalyticalJsonHelp.ModelGuidDescription);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Links",
                "The link of each element after the conversion as JSON (null when it could not be converted).");

            OutErrorMessages(
                "Error message of each element (empty when it was converted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            if (!AnalyticalJsonHelp.TryAddElements(da, 0, parameters) ||
                !AnalyticalJsonHelp.TryAddModelId(this, da, 1, parameters))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var items = AnalyticalJsonHelp.SplitErrors(
                response,
                "links",
                out List<string> errors);

            da.SetDataList(0, AnalyticalJsonHelp.TextsOf(items, "link"));
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UpdateAnalyticalModel;

        public override Guid ComponentGuid =>
            new Guid("7e246d17-c20f-43be-9194-c752f09bfd04");
    }
}
