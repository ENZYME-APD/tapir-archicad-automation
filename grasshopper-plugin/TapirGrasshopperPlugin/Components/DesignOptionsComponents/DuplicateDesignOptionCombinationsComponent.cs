using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class DuplicateDesignOptionCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DuplicateDesignOptionCombinations";

        public DuplicateDesignOptionCombinationsComponent()
            : base(
                "DuplicateDesignOptionCombinations",
                "Duplicates the given design option combinations and returns the identifiers of the new combinations. " +
                "Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionCombinationGuids",
                "Identifiers of the design option combinations to duplicate.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "DesignOptionCombinationGuids",
                "Identifier of each new design option combination (null for failed items).");

            OutErrorMessages(
                "Error message of each combination (empty when it was duplicated).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return;
            }

            var input = DesignOptionCombinationsObject.ConvertFrom(wrappers);
            if (input == null)
            {
                this.AddError(
                    "Invalid design option combination identifier in the DesignOptionCombinationGuids input.");
                return;
            }

            SetCadValuesWithCreatedIds<DesignOptionCombinationGuid>(
                CommandName,
                input,
                ToAddOn,
                da,
                "designOptionCombinationIdsOrErrors",
                "designOptionCombinationId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DuplicateDesignOptionCombinations;

        public override Guid ComponentGuid =>
            new Guid("f853d099-68a3-4b29-908b-6bc4da50fac9");
    }
}
