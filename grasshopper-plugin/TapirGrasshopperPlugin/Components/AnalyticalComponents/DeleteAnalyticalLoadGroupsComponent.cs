using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class DeleteAnalyticalLoadGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteAnalyticalLoadGroups";

        public DeleteAnalyticalLoadGroupsComponent()
            : base(
                "DeleteAnalyticalLoadGroups",
                "Deletes the given structural analysis load groups in one undo step.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "LoadGroupGuids",
                "Identifiers of the load groups to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each load group (empty when it was deleted successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out LoadGroupIdsObject input))
            {
                this.AddError("Invalid identifier in the LoadGroupGuids input.");
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteAnalyticalLoadGroups;

        public override Guid ComponentGuid =>
            new Guid("b120df4c-66f7-495c-87b3-5a051acea018");
    }
}
