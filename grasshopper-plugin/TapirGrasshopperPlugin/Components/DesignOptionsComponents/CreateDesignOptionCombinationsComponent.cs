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
    public class CreateDesignOptionCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateDesignOptionCombinations";

        public CreateDesignOptionCombinationsComponent()
            : base(
                "CreateDesignOptionCombinations",
                "Create new design option combinations with the given parameters. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the design option combinations to create.");

            InGenericTree(
                "ActiveDesignOptionGuids",
                "Identifiers of the active design options for each combination (one branch per combination).");
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

            if (!da.TryGetTree(
                    1,
                    out GH_Structure<GH_ObjectWrapper> activeOptionsTree))
            {
                return;
            }

            var input = new CreateDesignOptionCombinationsParameters
            {
                DesignOptionCombinations = new List<CreateDesignOptionCombinationParameter>()
            };

            for (var i = 0; i < names.Count; i++)
            {
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

                input.DesignOptionCombinations.Add(
                    new CreateDesignOptionCombinationParameter
                    {
                        Name = names[i],
                        ActiveDesignOptions = activeDesignOptions
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateDesignOptionCombinations;

        public override Guid ComponentGuid =>
            new Guid("58c5b6d9-fde8-4bc5-a636-5b4d6e30f37c");
    }
}
