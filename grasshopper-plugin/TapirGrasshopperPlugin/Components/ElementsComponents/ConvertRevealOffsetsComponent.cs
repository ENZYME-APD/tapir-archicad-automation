using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ConvertRevealOffsetsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "ConvertRevealOffsets";

        public ConvertRevealOffsetsComponent()
            : base(
                "ConvertRevealOffsets",
                "Converts the reveal depth offset of windows and doors placed in walls between the two " +
                "references Archicad uses: measured from the core skin of a composite wall, or from the wall " +
                "side. Give either CoreOffsets or SideOffsets; both values are returned. For walls with a " +
                "basic structure the two values are equal. The output is aligned with the input list of " +
                "openings. Available from Archicad 27.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Windows and doors placed in walls.");

            InNumbers(
                "CoreOffsets",
                "Reveal depth offset of each opening measured from the core skin of the owner wall, to " +
                "convert to the wall side reference (input only 1 to use the same value for all). Give " +
                "either this or SideOffsets.");

            InNumbers(
                "SideOffsets",
                "Reveal depth offset of each opening measured from the side of the owner wall, to convert " +
                "to the core skin reference (input only 1 to use the same value for all). Give either this " +
                "or CoreOffsets.");

            SetOptionality(
                new[]
                {
                    1,
                    2
                });
        }

        protected override void AddOutputs()
        {
            OutNumberList(
                "CoreOffsets",
                "Reveal depth offset of each opening measured from the core skin of the owner wall.");

            OutNumberList(
                "SideOffsets",
                "Reveal depth offset of each opening measured from the side of the owner wall.");

            OutErrorMessages(
                "Error message of each opening (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return;
            }

            var elementIds = ElementInputHelp.ElementGuids(
                this,
                wrappers,
                "ElementGuids");
            if (elementIds == null)
            {
                return;
            }

            var coreOffsets = da.GetOptionals<double>(1);
            var sideOffsets = da.GetOptionals<double>(2);
            if ((coreOffsets.Count > 0) == (sideOffsets.Count > 0))
            {
                this.AddError("Give exactly one of the inputs CoreOffsets and SideOffsets.");
                return;
            }

            var offsetKey = coreOffsets.Count > 0 ? "coreOffset" : "sideOffset";
            var offsets = coreOffsets.Count > 0 ? coreOffsets : sideOffsets;
            if (!ElementInputHelp.CheckOneOrMany(
                    this,
                    offsets,
                    elementIds.Count,
                    coreOffsets.Count > 0 ? "CoreOffsets" : "SideOffsets"))
            {
                return;
            }

            var openings = new JArray();
            for (var i = 0; i < elementIds.Count; i++)
            {
                openings.Add(
                    new JObject
                    {
                        ["elementId"] = JObject.FromObject(elementIds[i]),
                        [offsetKey] = ElementInputHelp.ItemAt(
                            offsets,
                            i)
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["openings"] = openings },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var resultCoreOffsets = new List<object>();
            var resultSideOffsets = new List<object>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "revealOffsets"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    resultCoreOffsets.Add(null);
                    resultSideOffsets.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                resultCoreOffsets.Add(
                    JsonOutputHelp.Scalar(
                        item,
                        "coreOffset"));
                resultSideOffsets.Add(
                    JsonOutputHelp.Scalar(
                        item,
                        "sideOffset"));
                errors.Add("");
            }

            da.SetDataList(
                0,
                resultCoreOffsets);
            da.SetDataList(
                1,
                resultSideOffsets);
            da.SetDataList(
                2,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ConvertRevealOffsets;

        public override Guid ComponentGuid =>
            new Guid("62e8c000-0ff8-4b9b-ac21-aed7d2c74b54");
    }
}
