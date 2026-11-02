using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class ActivateDesignOptionsInCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ActivateDesignOptionsInCombinations";

        public ActivateDesignOptionsInCombinationsComponent()
            : base(
                "ActivateDesignOptionsInCombinations",
                "Activates one design option in each of the given combinations, without listing all active design " +
                "options of the combination. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionCombinationGuids",
                "Identifiers of the design option combinations to modify.");

            InGenerics(
                "DesignOptionGuids",
                "Identifier of the design option to activate in each combination. " +
                "Input only 1 to activate the same design option in all combinations.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each combination (empty when the design option was activated).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> combinationWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> optionWrappers))
            {
                return;
            }

            if (optionWrappers.Count != 1 &&
                optionWrappers.Count != combinationWrappers.Count)
            {
                this.AddError(
                    "DesignOptionGuids must hold 1 item or as many items as DesignOptionCombinationGuids.");
                return;
            }

            var designOptionsToActivate = new JArray();
            for (var i = 0; i < combinationWrappers.Count; i++)
            {
                var combinationId = GuidObject<DesignOptionCombinationGuid>
                    .CreateFromWrapper(combinationWrappers[i]);
                if (combinationId == null)
                {
                    this.AddError(
                        "Invalid design option combination identifier in the DesignOptionCombinationGuids input.");
                    return;
                }

                var optionId = GuidObject<DesignOptionGuid>.CreateFromWrapper(
                    optionWrappers[optionWrappers.Count == 1 ? 0 : i]);
                if (optionId == null)
                {
                    this.AddError(
                        "Invalid design option identifier in the DesignOptionGuids input.");
                    return;
                }

                designOptionsToActivate.Add(
                    new JObject
                    {
                        ["designOptionCombinationId"] = new JObject { ["guid"] = combinationId.Guid },
                        ["designOptionId"] = new JObject { ["guid"] = optionId.Guid }
                    });
            }

            var parameters = new JObject
            {
                ["designOptionsToActivate"] = designOptionsToActivate
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ActivateDesignOptionsInCombinations;

        public override Guid ComponentGuid =>
            new Guid("01599d2b-0ed7-4f63-9404-af6ab68f099d");
    }
}
