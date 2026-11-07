using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class CreateAnalyticalLoadCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateAnalyticalLoadCombinations";

        public CreateAnalyticalLoadCombinationsComponent()
            : base(
                "CreateAnalyticalLoadCombinations",
                "Creates structural analysis load combinations, optionally with their load factors, " +
                "in one undo step and returns their identifiers. " +
                "The combination type is available from Archicad 27.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each load combination to create.");

            InTexts(
                "Categories",
                "Category of each load combination (input only 1 to use the same category for all): " +
                AnalyticalLoadEnums.Categories + ".");

            InTexts(
                "Descriptions",
                "Description of each load combination (input only 1 to use the same description for all). " +
                "Optional; empty when it is omitted.");

            InTexts(
                "NationalStandards",
                "National standard of each load combination (input only 1 to use the same standard for all): " +
                AnalyticalLoadEnums.NationalStandards + ". Optional; not set when it is omitted.");

            InTexts(
                "Types",
                "Type of each load combination (input only 1 to use the same type for all): " +
                AnalyticalLoadEnums.CombinationTypes + ". Archicad 27 and newer. Optional; not set when it is omitted.");

            InGenericTree(
                "LoadCaseGuids",
                "Load cases of each load combination, one branch per load combination " +
                "(input only 1 branch to use the same load cases for all). " +
                "Optional; the combinations get no load cases when it is omitted.");

            inManager.AddNumberParameter(
                "Factors",
                "Factors",
                ("Factor of each load case, in the branches of LoadCaseGuids " +
                 "(1 branch for all combinations or one per combination; " +
                 "1 item for all load cases of the branch or one per load case). " +
                 "Optional; not set when it is omitted.")
                .WithTypeName("numberTree"),
                GH_ParamAccess.tree);

            inManager.AddNumberParameter(
                "Multipliers",
                "Multipliers",
                ("Multiplier of each load case, in the branches of LoadCaseGuids " +
                 "(1 branch for all combinations or one per combination; " +
                 "1 item for all load cases of the branch or one per load case). " +
                 "Optional; 1 when it is omitted.")
                .WithTypeName("numberTree"),
                GH_ParamAccess.tree);

            SetOptionality(
                new[]
                {
                    2,
                    3,
                    4,
                    5,
                    6,
                    7
                });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "LoadCombinationGuids",
                "Identifier of each created load combination (null when it could not be created).");

            OutErrorMessages(
                "Error message of each load combination (empty when it was created successfully).");
        }

        // The number of branches an optional number tree input holds; an
        // unconnected tree input may still hold one empty branch.
        private static int BranchCount(
            GH_Structure<GH_Number> tree)
        {
            return (tree?.DataCount ?? 0) == 0
                ? 0
                : tree.Branches.Count;
        }

        // The number branch of the given combination; empty when the tree
        // has no data. An empty item would shift the following values onto
        // the wrong load cases, so it is an error.
        private bool TryGetNumberBranch(
            GH_Structure<GH_Number> tree,
            int index,
            string inputName,
            out List<double> numbers)
        {
            numbers = new List<double>();
            if (BranchCount(tree) == 0)
            {
                return true;
            }
            var branch = tree.Branches[tree.Branches.Count == 1 ? 0 : index];
            if (branch.Any(x => x == null))
            {
                this.AddError(
                    $"The input {inputName} must not contain empty items.");
                return false;
            }
            numbers = branch.Select(x => x.Value).ToList();
            return true;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names) ||
                !da.TryGetList(
                    1,
                    out List<string> categories))
            {
                return;
            }

            da.TryGetList(
                2,
                out List<string> descriptions);
            da.TryGetList(
                3,
                out List<string> nationalStandards);
            da.TryGetList(
                4,
                out List<string> types);
            da.TryGetTree(
                5,
                out GH_Structure<IGH_Goo> loadCaseTree);
            da.TryGetTree(
                6,
                out GH_Structure<GH_Number> factorTree);
            da.TryGetTree(
                7,
                out GH_Structure<GH_Number> multiplierTree);

            // An unconnected tree input may still hold one empty branch.
            var loadCaseBranchCount = (loadCaseTree?.DataCount ?? 0) == 0
                ? 0
                : loadCaseTree.Branches.Count;
            if (!AnalyticalHelp.CheckRequiredCount(this, categories.Count, names.Count, "Categories", "Names") ||
                !AnalyticalHelp.CheckOptionalCount(this, descriptions.Count, names.Count, "Descriptions", "Names") ||
                !AnalyticalHelp.CheckOptionalCount(this, nationalStandards.Count, names.Count, "NationalStandards", "Names") ||
                !AnalyticalHelp.CheckOptionalCount(this, types.Count, names.Count, "Types", "Names"))
            {
                return;
            }

            if (loadCaseBranchCount > 1 &&
                loadCaseBranchCount != names.Count)
            {
                this.AddError(
                    "The number of branches in the input LoadCaseGuids must be 0, 1 or equal to the size of the input Names.");
                return;
            }

            var factorBranchCount = BranchCount(factorTree);
            var multiplierBranchCount = BranchCount(multiplierTree);
            if ((factorBranchCount > 1 && factorBranchCount != loadCaseBranchCount) ||
                (multiplierBranchCount > 1 && multiplierBranchCount != loadCaseBranchCount))
            {
                this.AddError(
                    "The inputs Factors and Multipliers must have 0 branches, 1 branch or as many branches as the input LoadCaseGuids.");
                return;
            }

            var loadCombinations = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var loadCombination = new JObject
                {
                    ["name"] = names[i],
                    ["category"] = AnalyticalHelp.ItemFor(categories, i)
                };
                if (descriptions.Count > 0)
                {
                    loadCombination["description"] = AnalyticalHelp.ItemFor(descriptions, i);
                }
                if (nationalStandards.Count > 0)
                {
                    loadCombination["nationalStandard"] = AnalyticalHelp.ItemFor(nationalStandards, i);
                }
                if (types.Count > 0)
                {
                    loadCombination["type"] = AnalyticalHelp.ItemFor(types, i);
                }

                if (loadCaseBranchCount > 0)
                {
                    var branchIndex = loadCaseBranchCount == 1 ? 0 : i;
                    if (!AnalyticalHelp.TryConvertIds(
                            this,
                            loadCaseTree.Branches[branchIndex],
                            "LoadCaseGuids",
                            out List<LoadCaseGuid> loadCaseIds))
                    {
                        return;
                    }

                    if (!TryGetNumberBranch(
                            factorTree,
                            branchIndex,
                            "Factors",
                            out List<double> factors) ||
                        !TryGetNumberBranch(
                            multiplierTree,
                            branchIndex,
                            "Multipliers",
                            out List<double> multipliers) ||
                        !AnalyticalHelp.TryBuildLoadFactors(
                            this,
                            loadCaseIds,
                            factors,
                            multipliers,
                            out JArray loadFactors))
                    {
                        return;
                    }

                    loadCombination["loadFactors"] = loadFactors;
                }

                loadCombinations.Add(loadCombination);
            }

            SetCadValuesWithCreatedIds<LoadCombinationGuid>(
                CommandName,
                new JObject { ["loadCombinations"] = loadCombinations },
                ToAddOn,
                da,
                "loadCombinationIds",
                "loadCombinationId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAnalyticalLoadCombinations;

        public override Guid ComponentGuid =>
            new Guid("fb91b1b3-92b5-434a-98c8-41b3a09f2028");
    }
}
