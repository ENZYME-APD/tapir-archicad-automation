using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class RenameGraphicOverrideRuleGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameGraphicOverrideRuleGroups";

        public RenameGraphicOverrideRuleGroupsComponent()
            : base(
                "RenameGraphicOverrideRuleGroups",
                "Renames graphic override rule groups, found by name. Their rules are kept. " +
                "Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "RuleGroupNames",
                "Current names of the rule groups to rename.");

            InTexts(
                "NewNames",
                "New name of each rule group. It must not be empty or used by another rule group.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each rule group (empty when it was renamed).");
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
                    out List<string> newNames))
            {
                return;
            }

            if (names.Count != newNames.Count)
            {
                this.AddError(
                    "The size of the inputs RuleGroupNames and NewNames must be equal.");
                return;
            }

            var ruleGroups = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                ruleGroups.Add(
                    new JObject
                    {
                        ["name"] = names[i] ?? "",
                        ["newName"] = newNames[i] ?? ""
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["ruleGroups"] = ruleGroups },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameGraphicOverrideRuleGroups;

        public override Guid ComponentGuid =>
            new Guid("78f38470-0042-4319-8a6e-3daecf143cf3");
    }
}
