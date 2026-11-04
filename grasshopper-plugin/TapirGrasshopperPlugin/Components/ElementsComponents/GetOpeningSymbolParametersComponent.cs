using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetOpeningSymbolParametersComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetOpeningSymbolParameters";

        public GetOpeningSymbolParametersComponent()
            : base(
                "GetOpeningSymbolParameters",
                "Returns the GDL parameters of the cut and uncut floor plan symbols of Opening elements, in " +
                "the format of GetGDLParametersOfElements, as one JSON array per symbol. A symbol the opening " +
                "does not have is left out (null). Available from Archicad 29.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "openings";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Opening elements (made with the Opening tool) to read.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "CutSymbolParameters",
                "The GDL parameters of the cut symbol of each opening, as a JSON array of objects with name, " +
                "type, value and for arrays dimension1 and dimension2 (null when the opening has no cut " +
                "symbol). SetOpeningSymbolParameters takes it back.");

            OutTexts(
                "UnCutSymbolParameters",
                "The GDL parameters of the uncut symbol of each opening, in the same format (null when the " +
                "opening has no uncut symbol).");

            OutErrorMessages(
                "Error message of each element (empty when it was read).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var cutParameters = new List<string>();
            var unCutParameters = new List<string>();
            var errors = new List<string>();
            foreach (var item in items)
            {
                if (JsonOutputHelp.IsError(item))
                {
                    cutParameters.Add(null);
                    unCutParameters.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                cutParameters.Add(ToText(item["cutSymbolParameters"]));
                unCutParameters.Add(ToText(item["unCutSymbolParameters"]));
                errors.Add("");
            }

            da.SetDataList(0, cutParameters);
            da.SetDataList(1, unCutParameters);
            da.SetDataList(2, errors);
        }

        private static string ToText(
            JToken parameters)
        {
            return parameters is JArray array
                ? array.ToString(Formatting.None)
                : null;
        }

        // Shares the icon of GetOpeningFloorPlanParameters until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetOpeningFloorPlanParameters;

        public override Guid ComponentGuid =>
            new Guid("82baa930-1cc7-45fb-a1d3-bf40532f363c");
    }
}
