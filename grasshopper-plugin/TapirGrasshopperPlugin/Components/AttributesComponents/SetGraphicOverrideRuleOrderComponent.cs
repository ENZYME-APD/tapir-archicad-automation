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
    public class SetGraphicOverrideRuleOrderComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetGraphicOverrideRuleOrder";

        public SetGraphicOverrideRuleOrderComponent()
            : base(
                "SetGraphicOverrideRuleOrder",
                "Sets the order of the rules in graphic override rule groups, found by name. The listed rules " +
                "come first, in the given order; the other rules of the group keep their order after them. " +
                "Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "RuleGroupNames",
                "Names of the rule groups to reorder.");

            InGenericTree(
                "RuleGuids",
                "Rules of each group in their new order (one branch per rule group, as " +
                "GetGraphicOverrideRuleGroups returns them). Each must be a rule of its group, listed once.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each rule group (empty when it was reordered).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> ruleGroupNames))
            {
                return;
            }

            // A generic parameter hands its tree out as IGH_Goo (#634).
            if (!da.TryGetTree(
                    1,
                    out GH_Structure<IGH_Goo> ruleTree))
            {
                return;
            }

            if (ruleTree.Branches.Count != ruleGroupNames.Count)
            {
                this.AddError(
                    "The branch count of the input RuleGuids must be equal to the size of the input RuleGroupNames.");
                return;
            }

            var ruleGroups = new JArray();
            for (var i = 0; i < ruleGroupNames.Count; i++)
            {
                var ruleIds = GraphicOverrideHelp.RuleIdItemsOf(ruleTree.Branches[i]);
                if (ruleIds == null)
                {
                    this.AddError("Invalid rule identifier in the RuleGuids input.");
                    return;
                }

                ruleGroups.Add(
                    new JObject
                    {
                        ["name"] = ruleGroupNames[i] ?? "",
                        ["graphicOverrideRuleIds"] = ruleIds
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["graphicOverrideRuleGroups"] = ruleGroups },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetGraphicOverrideRuleOrder;

        public override Guid ComponentGuid =>
            new Guid("dd02c011-7c18-4ab5-b860-7a6f83f2a5b8");
    }
}
