using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class DeleteGraphicOverrideRuleGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteGraphicOverrideRuleGroups";

        public DeleteGraphicOverrideRuleGroupsComponent()
            : base(
                "DeleteGraphicOverrideRuleGroups",
                "Deletes graphic override rule groups, found by name. A group that still contains rules is kept " +
                "unless its rules are moved to another group or deleted with it. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "RuleGroupNames",
                "Names of the rule groups to delete.");

            InText(
                "MoveRulesToGroup",
                "Name of the rule group the rules of the deleted groups are moved to, so they are kept. " +
                "Created when no rule group has this name. Optional.");

            // No default value, so the key is only sent when the input is given.
            inManager.AddBooleanParameter(
                "DeleteRules",
                "DeleteRules",
                ("Delete the rules of the deleted groups too. Without this or MoveRulesToGroup, a group that " +
                 "still contains rules is not deleted. Optional.").WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each rule group (empty when it was deleted).");
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

            var parameters = new JObject { ["ruleGroupNames"] = new JArray(ruleGroupNames) };

            if (da.TryGet(
                    1,
                    out string moveRulesToGroup) &&
                !string.IsNullOrEmpty(moveRulesToGroup))
            {
                parameters["moveRulesToGroup"] = moveRulesToGroup;
            }

            if (da.TryGet(
                    2,
                    out bool deleteRules))
            {
                parameters["deleteRules"] = deleteRules;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteGraphicOverrideRuleGroups;

        public override Guid ComponentGuid =>
            new Guid("622297c1-368b-449d-9449-1219e95bbee4");
    }
}
