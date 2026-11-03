using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GraphicOverrides;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class MoveGraphicOverrideRulesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "MoveGraphicOverrideRules";

        public MoveGraphicOverrideRulesComponent()
            : base(
                "MoveGraphicOverrideRules",
                "Moves graphic override rules to another rule group, or to another place in their own group. " +
                "The rules are processed in the order of the input. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "RuleGuids",
                "Identifiers of the rules to move.");

            InTexts(
                "RuleGroupNames",
                "Rule group each rule is moved to; it can be the rule's own group to change its place there. " +
                "Created when no rule group has this name (input only 1 to use the same group for all).");

            InGenerics(
                "BeforeRuleGuids",
                "Rule of the target group each moved rule is put in front of (input only 1 to use the same for all; " +
                "a null item puts that rule last). Optional: without it the rules become the last of their group.");

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each rule (empty when it was moved).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> ruleWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> ruleGroupNames))
            {
                return;
            }

            var beforeWrappers = da.GetOptionals<GH_ObjectWrapper>(2);

            if (!GraphicOverrideHelp.CheckCount(
                    this,
                    "RuleGroupNames",
                    ruleGroupNames.Count,
                    "RuleGuids",
                    ruleWrappers.Count) ||
                !GraphicOverrideHelp.CheckCount(
                    this,
                    "BeforeRuleGuids",
                    beforeWrappers.Count,
                    "RuleGuids",
                    ruleWrappers.Count))
            {
                return;
            }

            var rules = new JArray();
            for (var i = 0; i < ruleWrappers.Count; i++)
            {
                var ruleId = GraphicOverrideHelp.RuleIdObject(ruleWrappers[i]);
                if (ruleId == null)
                {
                    this.AddError("Invalid rule identifier in the RuleGuids input.");
                    return;
                }

                var rule = new JObject
                {
                    ["graphicOverrideRuleId"] = ruleId,
                    ["ruleGroupName"] = GraphicOverrideHelp.ValueFor(
                        ruleGroupNames,
                        i) ?? ""
                };

                var beforeWrapper = GraphicOverrideHelp.ValueFor(
                    beforeWrappers,
                    i);
                if (beforeWrapper?.Value != null)
                {
                    var beforeId = GraphicOverrideHelp.RuleIdObject(beforeWrapper);
                    if (beforeId == null)
                    {
                        this.AddError("Invalid rule identifier in the BeforeRuleGuids input.");
                        return;
                    }

                    rule["beforeGraphicOverrideRuleId"] = beforeId;
                }

                rules.Add(rule);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["graphicOverrideRules"] = rules },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveGraphicOverrideRules;

        public override Guid ComponentGuid =>
            new Guid("461db564-8c57-4ebd-b30d-b3c710f30c75");
    }
}
