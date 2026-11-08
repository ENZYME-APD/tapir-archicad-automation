using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GraphicOverrides;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class DeleteGraphicOverrideRulesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteGraphicOverrideRules";

        public DeleteGraphicOverrideRulesComponent()
            : base(
                "DeleteGraphicOverrideRules",
                "Deletes graphic override rules. They are also removed from the combinations that contain them. " +
                "Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "RuleGuids",
                "Identifiers of the rules to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each rule (empty when it was deleted).");
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

            var ruleIds = new JArray();
            foreach (var wrapper in ruleWrappers)
            {
                var ruleId = GraphicOverrideHelp.RuleIdObject(wrapper);
                if (ruleId == null)
                {
                    this.AddError("Invalid rule identifier in the RuleGuids input.");
                    return;
                }

                ruleIds.Add(new JObject { ["graphicOverrideRuleId"] = ruleId });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["graphicOverrideRuleIds"] = ruleIds },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteGraphicOverrideRules;

        public override Guid ComponentGuid =>
            new Guid("33f77f19-3611-4c87-85e4-c40a076789ff");
    }
}
