using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetOpeningFloorPlanParametersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetOpeningFloorPlanParameters";

        public SetOpeningFloorPlanParametersComponent()
            : base(
                "SetOpeningFloorPlanParameters",
                "Changes the floor plan parameters of Opening elements returned by GetOpeningFloorPlanParameters, " +
                "and sets or removes their cut and uncut symbols. Available from Archicad 29.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Opening elements (made with the Opening tool) to change.");

            InTexts(
                "FloorPlanParameters",
                OpeningSettingsHelp.FloorPlanParametersDescription +
                " Input only 1 to use the same settings for all.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each opening (empty when it was changed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out var elementIds))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> floorPlanTexts) ||
                !this.CheckSizes(
                    elementIds.Count,
                    "ElementGuids",
                    ("FloorPlanParameters", floorPlanTexts.Count)))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < elementIds.Count; i++)
            {
                if (!OpeningSettingsHelp.TryParse(
                        this,
                        floorPlanTexts.At(i),
                        "FloorPlanParameters",
                        OpeningSettingsHelp.FloorPlanKeys,
                        out var floorPlanParameters))
                {
                    return;
                }

                items.Add(new JObject
                {
                    ["elementId"] = elementIds[i],
                    ["floorPlanParameters"] = floorPlanParameters ?? new JObject()
                });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["openingsWithFloorPlanParameters"] = items },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetOpeningFloorPlanParameters;

        public override Guid ComponentGuid =>
            new Guid("acc61afb-1fc6-49f8-ae17-39e18e579c46");
    }
}
