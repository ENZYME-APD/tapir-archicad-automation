using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalReleasesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalReleases";

        public GetAnalyticalReleasesComponent()
            : base(
                "GetAnalyticalReleases",
                "Retrieves the releases of the given analytical members. Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements of the analytical members.");

            InGeneric(
                "AnalyticalModelGuid",
                AnalyticalJsonHelp.ModelGuidDescription);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Releases",
                "The release of each element as JSON (null when it could not be got); " +
                "it can be changed and fed to SetAnalyticalReleases.");

            OutErrorMessages(
                "Error message of each element (empty when its release was got).");
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
                "releases",
                out List<string> errors);

            da.SetDataList(0, AnalyticalJsonHelp.TextsOf(items, "release"));
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalVisibilitySettings;

        public override Guid ComponentGuid =>
            new Guid("9823ff9c-ee6c-4d9c-96c0-e29ac5f029d1");
    }
}
