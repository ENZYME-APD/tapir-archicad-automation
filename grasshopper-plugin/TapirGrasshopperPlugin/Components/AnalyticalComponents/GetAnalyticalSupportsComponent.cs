using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalSupportsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalSupports";

        public GetAnalyticalSupportsComponent()
            : base(
                "GetAnalyticalSupports",
                "Retrieves the settings, the supported member part and the geometry of the given analytical " +
                "supports. Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the analytical supports.");

            InGeneric(
                "AnalyticalModelGuid",
                "Identifier of the Structural Analytical Model the geometry is read from. Optional; the model " +
                "of the current view is used when it is omitted. Required in Archicad 26.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Supports",
                "The settings and supported member part of each support as JSON (null when it could not be got); " +
                "it can be fed to CreateAnalyticalSupports.");

            OutTexts(
                "Geometries",
                "The geometry of each support in the Structural Analytical Model as JSON " +
                "(coordinates, translation and rotation stiffness vectors).");

            OutErrorMessages(
                "Error message of each element (empty when its support was got).");
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
                "supports",
                out List<string> errors);

            da.SetDataList(0, AnalyticalJsonHelp.TextsOf(items, "support"));
            da.SetDataList(1, AnalyticalJsonHelp.TextsOf(items, "geometry"));
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalLoadCases;

        public override Guid ComponentGuid =>
            new Guid("ed7acd50-3c65-4995-9f54-24c6362ccaf8");
    }
}
