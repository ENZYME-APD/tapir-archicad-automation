using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GraphicOverrides;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetGraphicOverrideRulesByNameComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGraphicOverrideRulesByName";

        public GetGraphicOverrideRulesByNameComponent()
            : base(
                "GetGraphicOverrideRulesByName",
                "Gets graphic override rules by name, with their rule group, criterion and style. " +
                "The rule group narrows the search; without it the rule is searched in every rule group. " +
                "Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the rules to get. Rule names are unique within the project.");

            InTexts(
                "RuleGroupNames",
                "Rule group to search each rule in (input only 1 to use the same group for all). " +
                "Optional: without it every rule group is searched.");

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "IncludeCriterion",
                "Also return each rule's criterion as structured conditions (Criteria output), in the shape the " +
                "Criteria input of CreateGraphicOverrideRules and ModifyGraphicOverrideRules takes. Default is false.",
                false);

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "RuleGuids",
                "Identifier of each found rule.");

            OutTexts(
                "Names",
                "Name of each found rule.");

            OutTexts(
                "RuleGroupNames",
                "Name of the rule group of each found rule.");

            OutTexts(
                "CriterionXMLs",
                "Criterion of each found rule in the XML format Archicad saves override rules in.");

            OutTexts(
                "Styles",
                "Style of each found rule as JSON. " +
                GraphicOverrideHelp.StyleKeysDoc);

            OutErrorMessages(
                "Error message of each name (empty when the rule was found).");

            // Appended after the original outputs, so saved definitions keep their wiring.
            OutTexts(
                "Criteria",
                "Criterion of each found rule as structured conditions (JSON), in the shape the Criteria input of " +
                "CreateGraphicOverrideRules and ModifyGraphicOverrideRules takes. Only with IncludeCriterion; " +
                "null when the criterion XML is not in the format Archicad keeps rules in.");
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

            var ruleGroupNames = da.GetOptionals<string>(1);

            if (!GraphicOverrideHelp.CheckCount(
                    this,
                    "RuleGroupNames",
                    ruleGroupNames.Count,
                    "Names",
                    names.Count))
            {
                return;
            }

            var rules = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var rule = new JObject { ["name"] = names[i] ?? "" };
                var ruleGroupName = GraphicOverrideHelp.ValueFor(
                    ruleGroupNames,
                    i);
                if (!string.IsNullOrEmpty(ruleGroupName))
                {
                    rule["ruleGroupName"] = ruleGroupName;
                }

                rules.Add(rule);
            }

            var parameters = new JObject { ["graphicOverrideRules"] = rules };
            if (da.GetOptional(2, false))
            {
                parameters["includeCriterion"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var outputs = new GraphicOverrideRuleOutputs();
            foreach (var rule in JsonOutputHelp.Items(
                         response,
                         "graphicOverrideRules"))
            {
                outputs.Add(rule);
            }

            outputs.SetOutputs(
                da,
                0,
                true);

            da.SetDataList(
                6,
                outputs.Criteria);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetGraphicOverrideRulesByName;

        public override Guid ComponentGuid =>
            new Guid("e8328cbf-ccba-4929-bbfd-870b33f70935");
    }
}
