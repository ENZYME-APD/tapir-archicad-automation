using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class AddLoadCasesToLoadCombinationComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "AddLoadCasesToLoadCombination";

        public AddLoadCasesToLoadCombinationComponent()
            : base(
                "AddLoadCasesToLoadCombination",
                "Adds load cases with their factors to the given structural analysis load combination in one undo step.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "LoadCombinationGuid",
                "Identifier of the load combination to add the load cases to.");

            InGenerics(
                "LoadCaseGuids",
                "Identifiers of the load cases to add.");

            InNumbers(
                "Factors",
                "Factor of each load case (input only 1 to use the same factor for all). " +
                "Optional; not set when it is omitted.");

            InNumbers(
                "Multipliers",
                "Multiplier of each load case (input only 1 to use the same multiplier for all). " +
                "Optional; 1 when it is omitted.");

            SetOptionality(
                new[]
                {
                    2,
                    3
                });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each load case (empty when it was added successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out GH_ObjectWrapper loadCombinationWrapper))
            {
                return;
            }

            var loadCombinationId = GuidObject<LoadCombinationGuid>.CreateFromWrapper(loadCombinationWrapper);
            if (loadCombinationId == null)
            {
                this.AddError("Invalid identifier in the LoadCombinationGuid input.");
                return;
            }

            if (!AnalyticalHelp.TryGetIds(
                    this,
                    da,
                    1,
                    "LoadCaseGuids",
                    out List<LoadCaseGuid> loadCaseIds))
            {
                return;
            }

            da.TryGetList(
                2,
                out List<double> factors);
            da.TryGetList(
                3,
                out List<double> multipliers);

            if (!AnalyticalHelp.TryBuildLoadFactors(
                    this,
                    loadCaseIds,
                    factors,
                    multipliers,
                    out JArray loadFactors))
            {
                return;
            }

            var parameters = new JObject
            {
                ["loadCombinationId"] = AnalyticalHelp.IdOf(loadCombinationId),
                ["loadFactors"] = loadFactors
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AddLoadCasesToLoadCombination;

        public override Guid ComponentGuid =>
            new Guid("707852a6-07cc-4c1d-813e-0b1c0cc06112");
    }
}
