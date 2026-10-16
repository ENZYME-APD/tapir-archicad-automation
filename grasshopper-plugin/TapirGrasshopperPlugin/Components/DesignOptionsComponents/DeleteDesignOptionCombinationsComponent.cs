using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class DeleteDesignOptionCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteDesignOptionCombinations";

        public DeleteDesignOptionCombinationsComponent()
            : base(
                "DeleteDesignOptionCombinations",
                "Delete the given design option combinations. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionCombinationGuids",
                "Identifiers of the design option combinations to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each design option combination (empty when it was deleted successfully).");
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

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteDesignOptionCombinations;

        public override Guid ComponentGuid =>
            new Guid("4c88198a-c7d0-4131-b23f-861e2a353c7d");
    }
}
