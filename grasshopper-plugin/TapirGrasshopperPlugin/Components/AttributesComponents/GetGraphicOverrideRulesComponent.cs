using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GraphicOverrides;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetGraphicOverrideRulesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGraphicOverrideRules";

        public GetGraphicOverrideRulesComponent()
            : base(
                "GetGraphicOverrideRules",
                "Gets all graphic override rules of the project, with their rule group, criterion and style. " +
                "Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InBoolean(
                "IncludeCriterion",
                "Also return each rule's criterion as structured conditions (Criteria output), in the shape the " +
                "Criteria input of CreateGraphicOverrideRules and ModifyGraphicOverrideRules takes. Default is false.",
                false);

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "RuleGuids",
                "Identifier of each graphic override rule.");

            OutTexts(
                "Names",
                "Name of each rule.");

            OutTexts(
                "RuleGroupNames",
                "Name of the rule group of each rule.");

            OutTexts(
                "CriterionXMLs",
                "Criterion of each rule in the XML format Archicad saves override rules in. " +
                "Change its values and pass it to CreateGraphicOverrideRules or ModifyGraphicOverrideRules.");

            OutTexts(
                "Styles",
                "Style of each rule as JSON, in the shape the Styles input of CreateGraphicOverrideRules takes. " +
                GraphicOverrideHelp.StyleKeysDoc);

            // Appended after the original outputs, so saved definitions keep their wiring.
            OutTexts(
                "Criteria",
                "Criterion of each rule as structured conditions (JSON), in the shape the Criteria input of " +
                "CreateGraphicOverrideRules and ModifyGraphicOverrideRules takes. Only with IncludeCriterion; " +
                "null when the criterion XML is not in the format Archicad keeps rules in.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            if (da.GetOptional(0, false))
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
                false);

            da.SetDataList(
                5,
                outputs.Criteria);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetGraphicOverrideRules;

        public override Guid ComponentGuid =>
            new Guid("93325a99-8637-44a8-af17-1a656af4974f");
    }
}
