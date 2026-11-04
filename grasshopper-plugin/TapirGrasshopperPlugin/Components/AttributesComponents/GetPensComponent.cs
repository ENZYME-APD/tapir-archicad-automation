using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetPensComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetPens";

        public GetPensComponent()
            : base(
                "GetPens",
                "Get the given pens, or all of them, in effect in the current window: index, color, width and " +
                "description. It also works when the pens in effect are not a Pen Table attribute. " +
                "Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InIntegers(
                "Indices",
                "Indices of the pens to get, from 1 to the number of pens (GetCurrentPenTable returns PenCount). " +
                "Empty = every pen.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "Indices",
                "Index of each pen (null when failed).");

            OutColors(
                "Colors",
                "Color of each pen.");

            OutNumberList(
                "Widths",
                "Width of each pen.");

            OutTexts(
                "Descriptions",
                "Description of each pen.");

            OutErrorMessages(
                "Error message for each requested pen (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var indices = da.OptionalList<int>(0);

            var parameters = new JObject();
            if (indices.Count > 0)
            {
                parameters["indices"] = new JArray(indices);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var penIndices = new List<object>();
            var colors = new List<object>();
            var widths = new List<object>();
            var descriptions = new List<object>();
            var errors = new List<string>();

            foreach (var pen in JsonOutputHelp.Items(
                         response,
                         "pens"))
            {
                if (JsonOutputHelp.IsError(pen))
                {
                    penIndices.Add(null);
                    colors.Add(null);
                    widths.Add(null);
                    descriptions.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(pen));
                    continue;
                }

                penIndices.Add(JsonOutputHelp.Scalar(pen, "index"));
                colors.Add(JsonOutputHelp.ColorOf(pen["color"]));
                widths.Add(JsonOutputHelp.Scalar(pen, "width"));
                descriptions.Add(JsonOutputHelp.Scalar(pen, "description"));
                errors.Add("");
            }

            da.SetDataList(0, penIndices);
            da.SetDataList(1, colors);
            da.SetDataList(2, widths);
            da.SetDataList(3, descriptions);
            da.SetDataList(4, errors);
        }

        // Shares the icon of GetPenTables.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPenTables;

        public override Guid ComponentGuid =>
            new Guid("30a4f4d8-acf0-41cd-ac56-0ec0b4630d6a");
    }
}
