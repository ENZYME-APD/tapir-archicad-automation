using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class SetGraphicOverrideRuleGroupOrderComponent
        : ArchicadExecutorComponent
    {
        public override string CommandName =>
            "SetGraphicOverrideRuleGroupOrder";

        public SetGraphicOverrideRuleGroupOrderComponent()
            : base(
                "SetGraphicOverrideRuleGroupOrder",
                "Sets the order of the graphic override rule groups, found by name. The groups not listed keep " +
                "their order and follow the listed ones. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "RuleGroupNames",
                "Names of the rule groups in their new order.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> ruleGroupNames) ||
                ruleGroupNames.Count == 0)
            {
                this.AddError("RuleGroupNames is required.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["ruleGroupNames"] = new JArray(ruleGroupNames) },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            this.CheckExecutionResult(response);
        }

        // Shares the icon of SetGraphicOverrideRuleOrder.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetGraphicOverrideRuleOrder;

        public override Guid ComponentGuid =>
            new Guid("d6d7bfd5-1a80-48a5-91d1-78e68b0067c1");
    }
}
