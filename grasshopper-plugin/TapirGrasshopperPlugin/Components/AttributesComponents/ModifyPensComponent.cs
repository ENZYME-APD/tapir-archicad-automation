using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Drawing;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class ModifyPensComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyPens";

        public ModifyPensComponent()
            : base(
                "ModifyPens",
                "Change the color, width and description of single pens of the active Pen Table " +
                "(the one in effect for the model views or the layouts), keeping every other pen as it is. " +
                "Only the given fields are changed.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InIntegers(
                "PenIndices",
                "Indices (1..255) of the pens to change.");

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

            InTextWithDefault(
                "Target",
                "Which active Pen Table to change: \"model\" for the one in effect for the model views, " +
                "\"layout\" for the one in effect for the layouts.",
                "model");

            SetOptionality(new[] { 1, 2, 3, 4 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each pen (empty when it was changed successfully).");
        }

        private bool CheckSize(
            string inputName,
            int count,
            int penCount)
        {
            if (count > 1 && count != penCount)
            {
                this.AddError(
                    $"The size of the input {inputName} must be 0, 1 or equal to the size of the input PenIndices.");
                return false;
            }
            return true;
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
            if (!CheckSize("Colors", colors.Count, penIndices.Count) ||
                !CheckSize("Widths", widths.Count, penIndices.Count) ||
                !CheckSize("Descriptions", descriptions.Count, penIndices.Count))
            {
                return;
            }

            da.TryGet(4, out string target);
            if (string.IsNullOrEmpty(target))
            {
                target = "model";
            }
            if (target != "model" && target != "layout")
            {
                this.AddError("The input Target must be \"model\" or \"layout\".");
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
                    var color = colors.Count == 1 ? colors[0] : colors[i];
                    pen["color"] = new JObject
                    {
                        ["red"] = color.R / 255.0,
                        ["green"] = color.G / 255.0,
                        ["blue"] = color.B / 255.0
                    };
                }

                if (widths.Count > 0)
                {
                    pen["width"] = widths.Count == 1 ? widths[0] : widths[i];
                }

                if (descriptions.Count > 0)
                {
                    pen["description"] = descriptions.Count == 1 ? descriptions[0] : descriptions[i];
                }

                pens.Add(pen);
            }

            var parameters = new JObject
            {
                ["pens"] = pens,
                ["target"] = target
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetDataList(0, ExecutionResultsResponse.ErrorMessages(response));
        }

        // Shares the icon of CreatePenTables until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreatePenTables;

        public override Guid ComponentGuid =>
            new Guid("6f3b9c2e-4d71-4a8e-9b5c-2e8d7f1a3c40");
    }
}
