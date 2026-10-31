using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GraphicOverrides;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateGraphicOverrideRulesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateGraphicOverrideRules";

        public CreateGraphicOverrideRulesComponent()
            : base(
                "CreateGraphicOverrideRules",
                "Creates graphic override rules. The criterion is given as Archicad's criterion XML or as " +
                "structured conditions. The rule group is created when no rule group has the given name. " +
                "Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the rules to create, unique within the project.");

            InTexts(
                "RuleGroupNames",
                "Rule group to put each rule in, created when it does not exist " +
                "(input only 1 to use the same group for all).");

            InTexts(
                "CriterionXMLs",
                "Criterion of each rule in the XML format Archicad saves override rules in " +
                "(input only 1 to use the same for all; an empty item leaves it out for that rule). " +
                "Take it from GetGraphicOverrideRules and change its values: Archicad drops XML written from " +
                "scratch without an error. Optional: without it and Criteria, Archicad's default criterion is used.");

            InTexts(
                "Criteria",
                GraphicOverrideHelp.CriterionDoc +
                " Input only 1 to use the same for all; an empty item leaves it out for that rule. Optional.");

            InTexts(
                "Styles",
                "Style of each rule as JSON, e.g. {\"surfaceColor\": {\"red\": 1, \"green\": 0, \"blue\": 0}} " +
                "(input only 1 to use the same for all; an empty item leaves it out for that rule). " +
                GraphicOverrideHelp.StyleKeysDoc + " Optional.");

            SetOptionality(new[] { 2, 3, 4 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "RuleGuids",
                "Identifier of each created rule (null when it failed).");

            OutErrorMessages(
                "Error message of each rule (empty when it was created).");
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

            if (!da.TryGetList(
                    1,
                    out List<string> ruleGroupNames))
            {
                return;
            }

            var criterionXMLs = da.GetOptionals<string>(2);
            var criteria = da.GetOptionals<string>(3);
            var styles = da.GetOptionals<string>(4);

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("RuleGroupNames", ruleGroupNames.Count),
                         ("CriterionXMLs", criterionXMLs.Count),
                         ("Criteria", criteria.Count),
                         ("Styles", styles.Count)
                     })
            {
                if (!GraphicOverrideHelp.CheckCount(
                        this,
                        pair.Name,
                        pair.Count,
                        "Names",
                        names.Count))
                {
                    return;
                }
            }

            var rules = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var rule = new JObject
                {
                    ["name"] = names[i] ?? "",
                    ["ruleGroupName"] = GraphicOverrideHelp.ValueFor(
                        ruleGroupNames,
                        i) ?? ""
                };

                var criterionXML = GraphicOverrideHelp.ValueFor(
                    criterionXMLs,
                    i);
                if (!string.IsNullOrEmpty(criterionXML))
                {
                    rule["criterionXML"] = criterionXML;
                }

                var criterion = GraphicOverrideHelp.ValueFor(
                    criteria,
                    i);
                if (!string.IsNullOrWhiteSpace(criterion))
                {
                    if (!GraphicOverrideHelp.TryParseObject(
                            this,
                            "Criteria",
                            criterion,
                            out var criterionObject))
                    {
                        return;
                    }

                    rule["criterion"] = criterionObject;
                }

                var style = GraphicOverrideHelp.ValueFor(
                    styles,
                    i);
                if (!string.IsNullOrWhiteSpace(style))
                {
                    if (!GraphicOverrideHelp.TryParseObject(
                            this,
                            "Styles",
                            style,
                            out var styleObject))
                    {
                        return;
                    }

                    rule["style"] = styleObject;
                }

                rules.Add(rule);
            }

            SetCadValuesWithCreatedIds<GraphicOverrideRuleGuid>(
                CommandName,
                new JObject { ["graphicOverrideRules"] = rules },
                ToAddOn,
                da,
                "graphicOverrideRuleIds",
                "graphicOverrideRuleId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateGraphicOverrideRules;

        public override Guid ComponentGuid =>
            new Guid("f0e596f7-4317-4246-adb6-60f6260996ba");
    }
}
