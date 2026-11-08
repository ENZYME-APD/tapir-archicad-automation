using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class ConvertUnitValuesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "ConvertUnitValues";

        public ConvertUnitValuesComponent()
            : base(
                "ConvertUnitValues",
                "Formats lengths, areas, volumes and angles (given in meters, square meters, cubic meters and radians) " +
                "as Archicad shows them with the unit and rounding of the given Project Preference, returning the " +
                "text, its extra accuracy decimals and the unit. The outputs are aligned with the input values. " +
                "Available from Archicad 27.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InNumbers(
                "Values",
                "The values to convert, in meters, square meters, cubic meters or radians, matching the kind of the preference.");

            InTexts(
                "Preferences",
                "The Project Preference whose unit and rounding are applied to each value (input only 1 to use the " +
                "same for all): WorkModel, WorkLayout and WorkAngle are the Working Units; DimLinear, DimAngle, " +
                "DimRadial, DimLevel, DimElevation, DimDoorWindow (door, window and skylight), DimParapet (sill height) " +
                "and DimArea (area calculations) are the Dimensions; CalcLength, CalcArea, CalcVolume and CalcAngle are " +
                "the Calculation Units & Rules.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "ConvertedValues",
                "Each value as Archicad shows it in the unit of the preference, without the extra accuracy decimals. " +
                "It may contain the unit (null where the conversion failed).");

            OutTexts(
                "ExtraAccuracies",
                "The extra accuracy decimals of each value (at most two); empty when there are none.");

            OutTexts(
                "Units",
                "The unit of the preference of each value. It may be empty.");

            OutErrorMessages(
                "Error message of each value (empty when it was converted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<double> values))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> preferences))
            {
                return;
            }

            preferences = this.ExpandToCount(
                preferences,
                values.Count,
                "Preferences",
                "Values");
            if (preferences == null)
            {
                return;
            }

            var valuesToConvert = new JArray();
            for (var i = 0; i < values.Count; i++)
            {
                valuesToConvert.Add(
                    new JObject
                    {
                        ["value"] = values[i],
                        ["preference"] = preferences[i]
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["values"] = valuesToConvert },
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var convertedValues = new List<string>();
            var extraAccuracies = new List<string>();
            var units = new List<string>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "convertedValues"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    convertedValues.Add(null);
                    extraAccuracies.Add(null);
                    units.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                convertedValues.Add(item["convertedValue"]?.ToString());
                extraAccuracies.Add(item["extraAccuracy"]?.ToString());
                units.Add(item["unit"]?.ToString());
                errors.Add("");
            }

            da.SetDataList(0, convertedValues);
            da.SetDataList(1, extraAccuracies);
            da.SetDataList(2, units);
            da.SetDataList(3, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ConvertUnitValues;

        public override Guid ComponentGuid =>
            new Guid("21c46b54-880b-45b5-90ee-2835fc376abf");
    }
}
