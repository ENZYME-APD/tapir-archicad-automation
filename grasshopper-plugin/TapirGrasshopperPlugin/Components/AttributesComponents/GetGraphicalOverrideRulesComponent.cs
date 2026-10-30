using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GraphicOverrides;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetGraphicalOverrideRulesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGraphicalOverrideRules";

        public GetGraphicalOverrideRulesComponent()
            : base(
                "GetGraphicalOverrideRules",
                "Returns every graphic override rule with its criterion XML. Needs Archicad 27 or later. " +
                "GetGraphicOverrideRules also returns the style of the rules.",
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
                "Criterion of each rule in Archicad's own XML format. Change its values and pass it to " +
                "CreateGraphicalOverrideRule to make similar rules.");
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

            var ruleGuids = new List<GraphicOverrideRuleGuidWrapper>();
            var names = new List<string>();
            var ruleGroupNames = new List<string>();
            var criterionXMLs = new List<string>();

            foreach (var rule in JsonOutputHelp.Items(
                         response,
                         "rules"))
            {
                ruleGuids.Add(GraphicOverrideHelp.RuleIdOf(rule["ruleId"]));
                names.Add(rule["name"]?.ToString());
                ruleGroupNames.Add(rule["ruleGroupName"]?.ToString());
                criterionXMLs.Add(rule["criterionXML"]?.ToString());
            }

            da.SetDataList(0, ruleGuids);
            da.SetDataList(1, names);
            da.SetDataList(2, ruleGroupNames);
            da.SetDataList(3, criterionXMLs);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetGraphicalOverrideRules;

        public override Guid ComponentGuid =>
            new Guid("3d87bb26-8669-4392-9aaf-f73f687faa54");
    }
}
