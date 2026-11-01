using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateGraphicOverrideRuleGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateGraphicOverrideRuleGroups";

        public CreateGraphicOverrideRuleGroupsComponent()
            : base(
                "CreateGraphicOverrideRuleGroups",
                "Creates empty graphic override rule groups. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "RuleGroupNames",
                "Names of the rule groups to create, unique within the project.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each rule group (empty when it was created).");
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

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["ruleGroupNames"] = new JArray(ruleGroupNames) },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateGraphicOverrideRuleGroups;

        public override Guid ComponentGuid =>
            new Guid("42cd9131-5adb-4852-939b-ab68e13a8981");
    }
}
