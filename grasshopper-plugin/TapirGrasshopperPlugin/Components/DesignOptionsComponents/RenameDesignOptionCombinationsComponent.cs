using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class RenameDesignOptionCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameDesignOptionCombinations";

        public RenameDesignOptionCombinationsComponent()
            : base(
                "RenameDesignOptionCombinations",
                "Change the name of the given design option combinations. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionCombinationGuids",
                "Identifiers of the design option combinations to rename.");

            InTexts(
                "Names",
                "New name of each design option combination.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each design option combination (empty when it succeeded).");
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
                    out List<string> names))
            {
                return;
            }

            if (names.Count != combinationWrappers.Count)
            {
                this.AddError(
                    "The size of the inputs DesignOptionCombinationGuids and Names must be equal.");
                return;
            }

            var input = new RenameDesignOptionCombinationsParameters
            {
                DesignOptionCombinations = new List<RenameDesignOptionCombinationParameter>()
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

                if (string.IsNullOrEmpty(names[i]))
                {
                    this.AddError(
                        "Every item of the Names input needs a name.");
                    return;
                }

                input.DesignOptionCombinations.Add(
                    new RenameDesignOptionCombinationParameter
                    {
                        DesignOptionCombinationId = combinationId,
                        Name = names[i]
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameDesignOptionCombinations;

        public override Guid ComponentGuid =>
            new Guid("7bc8df49-c862-41c5-8a52-fac36dc171a5");
    }
}
