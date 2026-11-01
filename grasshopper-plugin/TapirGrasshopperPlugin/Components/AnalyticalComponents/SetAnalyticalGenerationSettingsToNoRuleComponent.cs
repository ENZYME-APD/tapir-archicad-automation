using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class SetAnalyticalGenerationSettingsToNoRuleComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetAnalyticalGenerationSettingsToNoRule";

        public SetAnalyticalGenerationSettingsToNoRuleComponent()
            : base(
                "SetAnalyticalGenerationSettingsToNoRule",
                "Sets the generation settings of the current Structural Analytical Model to no rules. " +
                "Available from Archicad 27.",
                GroupNames.Analytical)
        {
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

            AnalyticalHelp.ReportExecutionResult(
                this,
                response,
                "Failed to set the Structural Analytical Model generation settings to no rule.");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetAnalyticalGenerationSettingsToNoRule;

        public override Guid ComponentGuid =>
            new Guid("73e0b74d-6c40-47ed-ba8c-a9648a76c9de");
    }
}
