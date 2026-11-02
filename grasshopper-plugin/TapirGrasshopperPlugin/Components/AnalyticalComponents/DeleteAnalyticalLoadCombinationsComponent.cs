using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class DeleteAnalyticalLoadCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteAnalyticalLoadCombinations";

        public DeleteAnalyticalLoadCombinationsComponent()
            : base(
                "DeleteAnalyticalLoadCombinations",
                "Deletes the given structural analysis load combinations in one undo step.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "LoadCombinationGuids",
                "Identifiers of the load combinations to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each load combination (empty when it was deleted successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out LoadCombinationIdsObject input))
            {
                this.AddError("Invalid identifier in the LoadCombinationGuids input.");
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteAnalyticalLoadCombinations;

        public override Guid ComponentGuid =>
            new Guid("d027beeb-3d94-4ba0-b2fa-5cf221378022");
    }
}
