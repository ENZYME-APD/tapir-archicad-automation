using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Drawing;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class SetPensComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetPens";

        public SetPensComponent()
            : base(
                "SetPens",
                "Change the color, width and description of the given pens in effect in the current window, " +
                "the pens GetPens returns. Only the given fields are changed; each pen gets its own error " +
                "message. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InIntegers(
                "PenIndices",
                "Indices of the pens to change, from 1 to the number of pens.");

            // No default color: an unconnected input must leave the colors of the pens as they are.
            inManager.AddColourParameter(
                "Colors",
                "Colors",
                "New color of each pen (input only 1 to use the same color for all). Optional.",
                GH_ParamAccess.list);

            InNumbers(
                "Widths",
                "New width of each pen in paper millimeters (input only 1 to use the same width for all). Optional.");

            InTexts(
                "Descriptions",
                "New description of each pen (input only 1 to use the same description for all). Optional.");

            SetOptionality(new[] { 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each pen (empty when it was changed successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<int> penIndices))
            {
                return;
            }
            if (penIndices.Count == 0)
            {
                this.AddError("The input PenIndices must contain at least one item.");
                return;
            }

            da.TryGetList(1, out List<Color> colors);
            da.TryGetList(2, out List<double> widths);
            da.TryGetList(3, out List<string> descriptions);
            if (!this.CheckSizes(
                    penIndices.Count,
                    "PenIndices",
                    ("Colors", colors.Count),
                    ("Widths", widths.Count),
                    ("Descriptions", descriptions.Count)))
            {
                return;
            }

            var pens = new JArray();
            for (var i = 0; i < penIndices.Count; i++)
            {
                var pen = new JObject
                {
                    ["index"] = penIndices[i]
                };

                if (colors.Count > 0)
                {
                    var color = colors.At(i);
                    pen["color"] = new JObject
                    {
                        ["red"] = color.R / 255.0,
                        ["green"] = color.G / 255.0,
                        ["blue"] = color.B / 255.0
                    };
                }

                if (widths.Count > 0)
                {
                    pen["width"] = widths.At(i);
                }

                if (descriptions.Count > 0)
                {
                    pen["description"] = descriptions.At(i);
                }

                pens.Add(pen);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["pens"] = pens },
                ToAddOn,
                da);
        }

        // Shares the icon of CreatePenTables until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreatePenTables;

        public override Guid ComponentGuid =>
            new Guid("4b13a40c-698a-4971-849e-31070b0f98e0");
    }
}
