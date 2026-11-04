using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalLinksComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalLinks";

        public GetAnalyticalLinksComponent()
            : base(
                "GetAnalyticalLinks",
                "Retrieves the settings, the connected member parts and the geometry of the given analytical " +
                "links. Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the analytical links.");

            InGeneric(
                "AnalyticalModelGuid",
                "Identifier of the Structural Analytical Model the geometry is read from. Optional; the model " +
                "of the current view is used when it is omitted. Required in Archicad 26.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Links",
                "The settings and connected member parts of each link as JSON (null when it could not be got); " +
                "it can be fed to CreateAnalyticalLinks.");

            OutTexts(
                "Geometries",
                "The geometry of each link in the Structural Analytical Model as JSON " +
                "(coordinates, translation and rotation stiffness vectors, curves).");

            OutErrorMessages(
                "Error message of each element (empty when its link was got).");
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
            da.SetDataList(1, AnalyticalJsonHelp.TextsOf(items, "geometry"));
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalLoadCases;

        public override Guid ComponentGuid =>
            new Guid("1d6cbc7f-99bb-4e7c-8e10-3a9bbd35a9bc");
    }
}
