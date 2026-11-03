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
    public class CreateGraphicOverrideCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateGraphicOverrideCombinations";

        public CreateGraphicOverrideCombinationsComponent()
            : base(
                "CreateGraphicOverrideCombinations",
                "Creates graphic override combinations from existing rules. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the combinations to create, unique within the project.");

            InGenericTree(
                "RuleGuids",
                "Rules of each combination in priority order (one branch per combination, " +
                "or a single branch to use the same rules for all). Optional: without it the combinations have no rules.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "CombinationGuids",
                "Identifier of each created combination (null when it failed).");

            OutErrorMessages(
                "Error message of each combination (empty when it was created).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return;
            }

            // A generic parameter hands its tree out as IGH_Goo; asking for GH_ObjectWrapper
            // makes GetDataTree throw (#634).
            da.TryGetTree(
                1,
                out GH_Structure<IGH_Goo> ruleTree);

            var branchCount = ruleTree?.Branches.Count ?? 0;
            if (!GraphicOverrideHelp.CheckCount(
                    this,
                    "RuleGuids (branch count)",
                    branchCount,
                    "Names",
                    names.Count))
            {
                return;
            }

            var combinations = new JArray();
            for (var i = 0; i < names.Count; i++)
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

                combinations.Add(
                    new JObject
                    {
                        ["name"] = names[i] ?? "",
                        ["graphicOverrideRuleIds"] = ruleIds
                    });
            }

            SetCadValuesWithCreatedIds<GraphicOverrideCombinationGuid>(
                CommandName,
                new JObject { ["graphicOverrideCombinations"] = combinations },
                ToAddOn,
                da,
                "graphicOverrideCombinationIds",
                "graphicOverrideCombinationId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateGraphicOverrideCombinations;

        public override Guid ComponentGuid =>
            new Guid("33ac6c03-8c55-43cd-95bd-6a3a7f3c16d1");
    }
}
