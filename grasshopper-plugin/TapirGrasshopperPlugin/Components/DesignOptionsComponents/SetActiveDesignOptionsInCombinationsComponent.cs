using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class SetActiveDesignOptionsInCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetActiveDesignOptionsInCombinations";

        public SetActiveDesignOptionsInCombinationsComponent()
            : base(
                "SetActiveDesignOptionsInCombinations",
                "Set the active design options in the given combinations. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionCombinationGuids",
                "Identifiers of the design option combinations to modify.");

            InGenericTree(
                "ActiveDesignOptionGuids",
                "Identifiers of the active design options for each combination (one branch per combination).");
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

            if (!da.TryGetTree(
                    1,
                    out GH_Structure<GH_ObjectWrapper> activeOptionsTree))
            {
                return;
            }

            var input = new SetActiveDesignOptionsInCombinationsParameters
            {
                ActiveDesignOptionsInCombinations = new List<ActiveDesignOptionsInCombination>()
            };

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

                var activeDesignOptions = new List<DesignOptionGuidWrapper>();
                if (i < activeOptionsTree.Branches.Count)
                {
                    foreach (var wrapper in activeOptionsTree.Branches[i])
                    {
                        var option = GuidWrapper<DesignOptionGuid, DesignOptionGuidWrapper>
                            .CreateFromGhWrapper(wrapper);
                        if (option == null)
                        {
                            this.AddError(
                                "Invalid design option identifier in the ActiveDesignOptionGuids input.");
                            return;
                        }
                        activeDesignOptions.Add(option);
                    }
                }

                input.ActiveDesignOptionsInCombinations.Add(
                    new ActiveDesignOptionsInCombination
                    {
                        DesignOptionCombinationId = combinationId,
                        ActiveDesignOptions = activeDesignOptions
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetActiveDesignOptionsInCombinations;

        public override Guid ComponentGuid =>
            new Guid("60f4989d-09f7-4c50-9bf4-cab70be7d809");
    }
}
