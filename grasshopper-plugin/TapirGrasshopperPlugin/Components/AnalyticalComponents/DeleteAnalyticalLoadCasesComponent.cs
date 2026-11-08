using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class DeleteAnalyticalLoadCasesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteAnalyticalLoadCases";

        public DeleteAnalyticalLoadCasesComponent()
            : base(
                "DeleteAnalyticalLoadCases",
                "Deletes the given structural analysis load cases in one undo step.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "LoadCaseGuids",
                "Identifiers of the load cases to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each load case (empty when it was deleted successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out LoadCaseIdsObject input))
            {
                this.AddError("Invalid identifier in the LoadCaseGuids input.");
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteAnalyticalLoadCases;

        public override Guid ComponentGuid =>
            new Guid("d9fd96a5-bdbb-4f47-b592-9794d5ad4d8f");
    }
}
