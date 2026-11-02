using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GraphicOverrides;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class ModifyGraphicOverrideRulesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyGraphicOverrideRules";

        public ModifyGraphicOverrideRulesComponent()
            : base(
                "ModifyGraphicOverrideRules",
                "Modifies the name, criterion or style of graphic override rules. The criterion is given as " +
                "Archicad's criterion XML or as structured conditions. A given style replaces the whole style " +
                "of the rule. Only the given inputs are changed. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "RuleGuids",
                "Identifiers of the rules to modify.");

            InTexts(
                "NewNames",
                "New name of each rule (input only 1 to use the same for all; an empty item keeps the name). Optional.");

            InTexts(
                "CriterionXMLs",
                "New criterion of each rule in the XML format Archicad saves override rules in " +
                "(input only 1 to use the same for all; an empty item keeps the criterion). " +
                "Take it from GetGraphicOverrideRules and change its values. Optional.");

            InTexts(
                "Criteria",
                GraphicOverrideHelp.CriterionDoc +
                " Input only 1 to use the same for all; an empty item keeps the criterion. Optional.");

            InTexts(
                "Styles",
                "New style of each rule as JSON; it replaces the whole style of the rule " +
                "(input only 1 to use the same for all; an empty item keeps the style). " +
                GraphicOverrideHelp.StyleKeysDoc + " Optional.");

            SetOptionality(new[] { 1, 2, 3, 4 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each rule (empty when it was modified).");
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

            var newNames = da.GetOptionals<string>(1);
            var criterionXMLs = da.GetOptionals<string>(2);
            var criteria = da.GetOptionals<string>(3);
            var styles = da.GetOptionals<string>(4);

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("NewNames", newNames.Count),
                         ("CriterionXMLs", criterionXMLs.Count),
                         ("Criteria", criteria.Count),
                         ("Styles", styles.Count)
                     })
            {
                if (!GraphicOverrideHelp.CheckCount(
                        this,
                        pair.Name,
                        pair.Count,
                        "RuleGuids",
                        ruleWrappers.Count))
                {
                    return;
                }
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

                var rule = new JObject { ["graphicOverrideRuleId"] = ruleId };

                var newName = GraphicOverrideHelp.ValueFor(
                    newNames,
                    i);
                if (!string.IsNullOrEmpty(newName))
                {
                    rule["name"] = newName;
                }

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

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["graphicOverrideRules"] = rules },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyGraphicOverrideRules;

        public override Guid ComponentGuid =>
            new Guid("0b866d94-9001-4bd6-b1d6-aa23fc3259d0");
    }
}
