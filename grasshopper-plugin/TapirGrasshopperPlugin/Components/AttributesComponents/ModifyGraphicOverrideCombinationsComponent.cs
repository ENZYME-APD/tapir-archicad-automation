using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GraphicOverrides;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class ModifyGraphicOverrideCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyGraphicOverrideCombinations";

        public ModifyGraphicOverrideCombinationsComponent()
            : base(
                "ModifyGraphicOverrideCombinations",
                "Modifies the name and/or the rules of graphic override combinations. Omitted fields are kept. " +
                "Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "CombinationGuids",
                "Identifiers of the combinations to modify.");

            InTexts(
                "NewNames",
                "New name of each combination, unique within the project " +
                "(input only 1 to use the same for all; an empty item keeps the name). Optional.");

            InGenericTree(
                "RuleGuids",
                "New rules of each combination in priority order, replacing its current rules " +
                "(one branch per combination, or a single branch to use the same rules for all). " +
                "Optional: without it the current rules are kept.");

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each combination (empty when it was modified).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> combinationWrappers))
            {
                return;
            }

            var newNames = da.GetOptionals<string>(1);

            // A generic parameter hands its tree out as IGH_Goo (#634).
            da.TryGetTree(
                2,
                out GH_Structure<IGH_Goo> ruleTree);
            var hasRules = ruleTree != null && !ruleTree.IsEmpty;
            var branchCount = hasRules ? ruleTree.Branches.Count : 0;

            if (!GraphicOverrideHelp.CheckCount(
                    this,
                    "NewNames",
                    newNames.Count,
                    "CombinationGuids",
                    combinationWrappers.Count) ||
                !GraphicOverrideHelp.CheckCount(
                    this,
                    "RuleGuids (branch count)",
                    branchCount,
                    "CombinationGuids",
                    combinationWrappers.Count))
            {
                return;
            }

            var combinations = new JArray();
            for (var i = 0; i < combinationWrappers.Count; i++)
            {
                var combinationId = GraphicOverrideHelp.CombinationIdObject(combinationWrappers[i]);
                if (combinationId == null)
                {
                    this.AddError("Invalid combination identifier in the CombinationGuids input.");
                    return;
                }

                var combination = new JObject { ["graphicOverrideCombinationId"] = combinationId };

                var newName = GraphicOverrideHelp.ValueFor(
                    newNames,
                    i);
                if (!string.IsNullOrEmpty(newName))
                {
                    combination["name"] = newName;
                }

                if (hasRules)
                {
                    var ruleIds = GraphicOverrideHelp.RuleIdItemsOf(
                        GraphicOverrideHelp.BranchFor(
                            ruleTree,
                            i));
                    if (ruleIds == null)
                    {
                        this.AddError("Invalid rule identifier in the RuleGuids input.");
                        return;
                    }

                    combination["graphicOverrideRuleIds"] = ruleIds;
                }

                combinations.Add(combination);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["graphicOverrideCombinations"] = combinations },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyGraphicOverrideCombinations;

        public override Guid ComponentGuid =>
            new Guid("ba5422bf-3493-41f1-a9c8-83761a8453ae");
    }
}
