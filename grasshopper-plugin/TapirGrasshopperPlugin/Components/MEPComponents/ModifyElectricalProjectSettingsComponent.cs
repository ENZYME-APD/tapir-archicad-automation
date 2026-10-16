using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class ModifyElectricalProjectSettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyElectricalProjectSettings";

        public ModifyElectricalProjectSettingsComponent()
            : base(
                "ModifyElectricalProjectSettings",
                "Modify the electrical project settings: set the default voltage and add or remove wiring types and " +
                "load classifications. The removals happen first, so a name can be replaced in one run; adding an " +
                "existing name or removing an unknown one fails. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddNumberParameter(
                "Voltage",
                "Voltage",
                "New default voltage of the project. Optional.".WithTypeName("numberItem"),
                GH_ParamAccess.item);

            InTexts(
                "WiringTypeNamesToAdd",
                "Names of the wiring types to add. Optional.");

            InNumbers(
                "CrossSectionalAreasToAdd",
                "Cross-sectional area of the wire of each added wiring type in square millimeters " +
                "(input only 1 to use the same area for all).");

            InTexts(
                "WiringTypeNamesToRemove",
                "Names of the wiring types to remove. Optional.");

            InTexts(
                "LoadClassificationNamesToAdd",
                "Names of the load classifications to add. Optional.");

            InNumbers(
                "DemandFactorsToAdd",
                "Demand factor of each added load classification, between 0 and 1 " +
                "(input only 1 to use the same factor for all).");

            InTexts(
                "LoadClassificationNamesToRemove",
                "Names of the load classifications to remove. Optional.");

            SetOptionality(new[] { 0, 1, 2, 3, 4, 5, 6 });
        }

        // Pairs the names to add with their values: one value for every name,
        // or one value per name.
        private bool TryGetItemsToAdd(
            IGH_DataAccess da,
            int namesIndex,
            int valuesIndex,
            string valueKey,
            out JArray items)
        {
            items = new JArray();
            da.TryGetList(namesIndex, out List<string> names);
            names = names ?? new List<string>();
            da.TryGetList(valuesIndex, out List<double> values);
            values = values ?? new List<double>();

            if (names.Count == 0)
            {
                if (values.Count > 0)
                {
                    this.AddError(
                        $"The input {Params.Input[valuesIndex].Name} needs the input {Params.Input[namesIndex].Name}.");
                    return false;
                }
                return true;
            }
            if (values.Count != 1 &&
                values.Count != names.Count)
            {
                this.AddError(
                    $"The size of the input {Params.Input[valuesIndex].Name} must be 1 or equal to the size of the input {Params.Input[namesIndex].Name}.");
                return false;
            }

            for (var i = 0; i < names.Count; i++)
            {
                items.Add(
                    new JObject
                    {
                        ["name"] = names[i] ?? "",
                        [valueKey] = values[values.Count == 1 ? 0 : i]
                    });
            }
            return true;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            double voltage = 0.0;
            if (da.GetData(0, ref voltage))
            {
                parameters["voltage"] = voltage;
            }

            if (!TryGetItemsToAdd(da, 1, 2, "crossSectionalArea", out JArray wiringTypesToAdd) ||
                !TryGetItemsToAdd(da, 4, 5, "demandFactor", out JArray loadClassificationsToAdd))
            {
                return;
            }
            if (wiringTypesToAdd.Count > 0)
            {
                parameters["wiringTypesToAdd"] = wiringTypesToAdd;
            }
            if (loadClassificationsToAdd.Count > 0)
            {
                parameters["loadClassificationsToAdd"] = loadClassificationsToAdd;
            }

            da.TryGetList(3, out List<string> wiringTypesToRemove);
            if (wiringTypesToRemove != null &&
                wiringTypesToRemove.Count > 0)
            {
                parameters["wiringTypesToRemove"] = new JArray(wiringTypesToRemove);
            }

            da.TryGetList(6, out List<string> loadClassificationsToRemove);
            if (loadClassificationsToRemove != null &&
                loadClassificationsToRemove.Count > 0)
            {
                parameters["loadClassificationsToRemove"] = new JArray(loadClassificationsToRemove);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            // The command answers a single execution result: a failed
            // modification is reported in it instead of failing the call.
            if ((bool?)response["success"] != true)
            {
                this.AddError(
                    response["error"]?["message"]?.ToString() ??
                    "Failed to modify the electrical project settings.");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyElectricalProjectSettings;

        public override Guid ComponentGuid =>
            new Guid("094ef1ba-015e-46cb-9022-273bfc7c6c75");
    }
}
