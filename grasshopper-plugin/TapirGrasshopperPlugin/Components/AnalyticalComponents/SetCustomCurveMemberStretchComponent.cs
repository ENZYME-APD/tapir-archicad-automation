using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class SetCustomCurveMemberStretchComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetCustomCurveMemberStretch";

        public SetCustomCurveMemberStretchComponent()
            : base(
                "SetCustomCurveMemberStretch",
                "Sets the lengths added to the start and end of the given curve members, in one undo step. " +
                "Available from Archicad 29.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements of the curve members.");

            InNumbers(
                "BegAdditionalLengths",
                "The length to add to each curve member at its start " +
                "(input only 1 to use the same length for all).");

            InNumbers(
                "EndAdditionalLengths",
                "The length to add to each curve member at its end " +
                "(input only 1 to use the same length for all).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when its stretch was set).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var elementParameters = new JObject();
            if (!AnalyticalJsonHelp.TryAddElements(da, 0, elementParameters) ||
                !da.TryGetList(1, out List<double> begLengths) ||
                !da.TryGetList(2, out List<double> endLengths))
            {
                return;
            }
            var elements = (JArray)elementParameters["elements"];

            if (!AnalyticalHelp.CheckRequiredCount(this, begLengths.Count, elements.Count, "BegAdditionalLengths", "ElementGuids") ||
                !AnalyticalHelp.CheckRequiredCount(this, endLengths.Count, elements.Count, "EndAdditionalLengths", "ElementGuids"))
            {
                return;
            }

            var stretches = new JArray();
            for (var i = 0; i < elements.Count; i++)
            {
                stretches.Add(
                    new JObject
                    {
                        ["elementId"] = elements[i]["elementId"],
                        ["begAdditionalLength"] = AnalyticalHelp.ItemFor(begLengths, i),
                        ["endAdditionalLength"] = AnalyticalHelp.ItemFor(endLengths, i)
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["stretches"] = stretches },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetAnalyticalVisibilitySettings;

        public override Guid ComponentGuid =>
            new Guid("48f91bad-e9d7-415f-9c21-4613165f8ea6");
    }
}
