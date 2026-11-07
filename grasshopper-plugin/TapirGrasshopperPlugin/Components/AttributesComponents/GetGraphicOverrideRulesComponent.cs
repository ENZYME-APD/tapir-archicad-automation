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
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
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
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetGraphicOverrideRules;

        public override Guid ComponentGuid =>
            new Guid("93325a99-8637-44a8-af17-1a656af4974f");
    }
}
