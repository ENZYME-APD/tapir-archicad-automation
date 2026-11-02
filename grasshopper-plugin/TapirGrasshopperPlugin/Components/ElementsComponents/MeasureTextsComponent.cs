using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class MeasureTextsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "MeasureTexts";

        public MeasureTextsComponent()
            : base(
                "MeasureTexts",
                "Measures texts before placing them: the length of every line for a given font, style and height, " +
                "on paper in millimetres or in the model at the current drawing scale, and the font's capital size " +
                "factor. Creates nothing. Give the font in FontNames or FontIndices.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Texts",
                "The texts to measure. Newlines separate lines, each measured on its own.");

            InTexts(
                "FontNames",
                "Name of the font as Archicad's font list carries it (input only 1 to use the same value for all). " +
                "Optional when FontIndices is given.");

            InIntegers(
                "FontIndices",
                "Index of the font, as GetDetailsOfElements reports it in fontIndex " +
                "(input only 1 to use the same value for all). Optional when FontNames is given.");

            InNumbers(
                "Heights",
                "The character height in millimetres, as a Text's height (input only 1 to use the same value for all).");

            InBooleans(
                "Bold",
                "Bold style. Defaults to false (input only 1 to use the same value for all). Optional.");

            InBooleans(
                "Italic",
                "Italic style. Defaults to false (input only 1 to use the same value for all). Optional.");

            InBooleans(
                "Underline",
                "Underline style. Defaults to false (input only 1 to use the same value for all). Optional.");

            InNumbers(
                "Slant",
                "The slant angle of the characters in radians; pi/2 (the default) is upright " +
                "(input only 1 to use the same value for all). Optional.");

            InBooleans(
                "InModelUnits",
                "True: the lengths are model distances at the current drawing scale; false (the default): lengths " +
                "on paper in millimetres (input only 1 to use the same value for all). Optional.");

            SetOptionality(new[] { 1, 2, 4, 5, 6, 7, 8 });
        }

        protected override void AddOutputs()
        {
            OutNumberList(
                "Lengths",
                "The length of the longest line of each text.");

            OutIntegers(
                "LongestLineIndices",
                "The zero-based index of the longest line of each text (the first of them on a tie).");

            OutNumbers(
                "LineLengths",
                "The length of every line of each text, in order; an empty line is 0 (one branch per text).");

            OutNumberList(
                "CapitalSizeFactors",
                "The ratio of the font's capital size to its typographic size (null when Archicad does not return it).");

            OutErrorMessages(
                "Error message of each text (empty when it was measured).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> texts) ||
                !da.TryGetList(
                    3,
                    out List<double> heights))
            {
                return;
            }

            var fontNames = da.OptionalList<string>(1);
            var fontIndices = da.OptionalList<int>(2);
            var bold = da.OptionalList<bool>(4);
            var italic = da.OptionalList<bool>(5);
            var underline = da.OptionalList<bool>(6);
            var slant = da.OptionalList<double>(7);
            var inModelUnits = da.OptionalList<bool>(8);

            if (fontNames.Count == 0 &&
                fontIndices.Count == 0)
            {
                this.AddError("Give the font in FontNames or FontIndices.");
                return;
            }

            if (!this.CheckSizes(
                    texts.Count,
                    "Texts",
                    ("FontNames", fontNames.Count),
                    ("FontIndices", fontIndices.Count),
                    ("Heights", heights.Count),
                    ("Bold", bold.Count),
                    ("Italic", italic.Count),
                    ("Underline", underline.Count),
                    ("Slant", slant.Count),
                    ("InModelUnits", inModelUnits.Count)))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < texts.Count; i++)
            {
                var item = new JObject
                {
                    ["text"] = texts[i] ?? "",
                    ["height"] = heights.At(i)
                };

                if (fontNames.Count > 0)
                {
                    item["fontName"] = fontNames.At(i);
                }

                if (fontIndices.Count > 0)
                {
                    item["fontIndex"] = fontIndices.At(i);
                }

                if (bold.Count > 0)
                {
                    item["bold"] = bold.At(i);
                }

                if (italic.Count > 0)
                {
                    item["italic"] = italic.At(i);
                }

                if (underline.Count > 0)
                {
                    item["underline"] = underline.At(i);
                }

                if (slant.Count > 0)
                {
                    item["slant"] = slant.At(i);
                }

                if (inModelUnits.Count > 0)
                {
                    item["inModelUnits"] = inModelUnits.At(i);
                }

                items.Add(item);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["texts"] = items },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var lengths = new List<double?>();
            var longestLineIndices = new List<int?>();
            var lineLengths = new DataTree<double>();
            var capitalSizeFactors = new List<double?>();
            var errors = new List<string>();

            var measurements = JsonOutputHelp.Items(
                response,
                "measurements");
            for (var i = 0; i < measurements.Count; i++)
            {
                var measurement = measurements[i];
                var path = new GH_Path(i);
                lineLengths.EnsurePath(path);

                if (JsonOutputHelp.IsError(measurement))
                {
                    lengths.Add(null);
                    longestLineIndices.Add(null);
                    capitalSizeFactors.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(measurement));
                    continue;
                }

                lengths.Add(measurement.Value<double?>("length"));
                longestLineIndices.Add(measurement.Value<int?>("longestLineIndex"));
                if (measurement["lineLengths"] is JArray lineLengthArray)
                {
                    foreach (var lineLength in lineLengthArray)
                    {
                        lineLengths.Add((double) lineLength, path);
                    }
                }

                capitalSizeFactors.Add(measurement.Value<double?>("capitalSizeFactor"));
                errors.Add("");
            }

            da.SetDataList(0, lengths);
            da.SetDataList(1, longestLineIndices);
            da.SetDataTree(2, lineLengths);
            da.SetDataList(3, capitalSizeFactors);
            da.SetDataList(4, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MeasureTexts;

        public override Guid ComponentGuid =>
            new Guid("2c4bc1fd-6def-452b-8259-d54a9702d49b");
    }
}
