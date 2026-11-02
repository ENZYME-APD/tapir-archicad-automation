using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetOpeningFloorPlanParametersComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetOpeningFloorPlanParameters";

        public GetOpeningFloorPlanParametersComponent()
            : base(
                "GetOpeningFloorPlanParameters",
                "Returns the floor plan parameters of Opening elements: the floor plan display mode, the divided " +
                "opening mode, the open edge style, the reference axis, the cut surface and outline lines, the " +
                "cover fills and the cut and uncut symbols, as one JSON object per opening. Available from " +
                "Archicad 29.",
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
                "FloorPlanParameters",
                "The floor plan parameters of each opening, as a JSON object (SetOpeningFloorPlanParameters takes it back).");

            OutErrorMessages(
                "Error message of each element (empty when it was read).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var floorPlanParameters = new List<string>();
            var errors = new List<string>();
            foreach (var item in items)
            {
                if (JsonOutputHelp.IsError(item))
                {
                    floorPlanParameters.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                floorPlanParameters.Add(OpeningSettingsHelp.ToText(item["floorPlanParameters"]));
                errors.Add("");
            }

            da.SetDataList(0, floorPlanParameters);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetOpeningFloorPlanParameters;

        public override Guid ComponentGuid =>
            new Guid("d8f4ac33-1806-42de-8c05-d7740e1e7035");
    }
}
