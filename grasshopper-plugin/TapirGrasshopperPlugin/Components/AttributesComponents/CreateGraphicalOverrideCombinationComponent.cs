using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GraphicOverrides;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateGraphicalOverrideCombinationComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateGraphicalOverrideCombination";

        public CreateGraphicalOverrideCombinationComponent()
            : base(
                "CreateGraphicalOverrideCombination",
                "Creates a graphic override combination from rule names, or replaces the rules of an existing one. " +
                "Needs Archicad 27 or later.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name of the combination. When a combination with this name exists, its rule list is replaced.");

            InTexts(
                "RuleNames",
                "Names of the rules in the combination, in order. Rule names are unique in the project, whatever " +
                "the rule group. Optional: without it the combination has no rules.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "CombinationGuid",
                "Identifier of the created or changed combination.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string name))
            {
                return;
            }

            var ruleNames = da.GetOptionals<string>(1);

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject
                    {
                        ["name"] = name,
                        ["ruleNames"] = new JArray(ruleNames)
                    },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                GraphicOverrideHelp.CombinationIdOf(response["combinationId"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateGraphicalOverrideCombination;

        public override Guid ComponentGuid =>
            new Guid("45cc22a0-232f-4bc8-819e-939b39709689");
    }
}
