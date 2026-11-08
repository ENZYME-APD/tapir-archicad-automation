using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class DeactivateDesignOptionsInCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeactivateDesignOptionsInCombinations";

        public DeactivateDesignOptionsInCombinationsComponent()
            : base(
                "DeactivateDesignOptionsInCombinations",
                "Turns off the given design options in the given combinations, leaving their other options as they are. " +
                "Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionCombinationGuids",
                "Identifiers of the design option combinations to modify.");

            InGenericTree(
                "DesignOptionGuids",
                "Identifiers of the design options to turn off in each combination, one branch per combination. " +
                "Input only 1 branch to turn off the same design options in all combinations.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each combination (empty when it succeeded).");
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

            // A generic parameter hands its tree out as IGH_Goo; asking for
            // GH_ObjectWrapper makes GetDataTree throw (#634).
            if (!da.TryGetTree(
                    1,
                    out GH_Structure<IGH_Goo> optionsTree))
            {
                return;
            }

            if (optionsTree.Branches.Count != 1 &&
                optionsTree.Branches.Count != combinationWrappers.Count)
            {
                this.AddError(
                    "DesignOptionGuids must hold 1 branch or as many branches as DesignOptionCombinationGuids.");
                return;
            }

            var deactivated = new JArray();
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

                var designOptions = new JArray();
                var branch = optionsTree.Branches[optionsTree.Branches.Count == 1 ? 0 : i];
                foreach (var goo in branch)
                {
                    var wrapper = goo as GH_ObjectWrapper ?? new GH_ObjectWrapper(goo);
                    var optionId = GuidObject<DesignOptionGuid>.CreateFromWrapper(wrapper);
                    if (optionId == null)
                    {
                        this.AddError(
                            "Invalid design option identifier in the DesignOptionGuids input.");
                        return;
                    }

                    designOptions.Add(
                        new JObject
                        {
                            ["designOptionId"] = new JObject { ["guid"] = optionId.Guid }
                        });
                }

                deactivated.Add(
                    new JObject
                    {
                        ["designOptionCombinationId"] = new JObject { ["guid"] = combinationId.Guid },
                        ["designOptions"] = designOptions
                    });
            }

            var parameters = new JObject
            {
                ["deactivatedDesignOptionsInCombinations"] = deactivated
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeactivateDesignOptionsInCombinations;

        public override Guid ComponentGuid =>
            new Guid("96c8ea43-1627-4dde-bcaa-53f5a2e20385");
    }
}
