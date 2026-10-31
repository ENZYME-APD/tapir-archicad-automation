using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class DeleteProjectInfoGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteProjectInfoGroups";

        public DeleteProjectInfoGroupsComponent()
            : base(
                "DeleteProjectInfoGroups",
                "Delete the given Project Info groups. A group that contains fixed (built-in) fields cannot be deleted. Available from Archicad 30.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "GroupGuids",
                "Identifiers of the Project Info groups to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each Project Info group (empty when it was deleted successfully).");
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

            var input = ProjectInfoGroupsObject.ConvertFrom(wrappers);
            if (input == null)
            {
                this.AddError(
                    "Invalid Project Info group identifier in the GroupGuids input.");
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteProjectInfoGroups;

        public override Guid ComponentGuid =>
            new Guid("5a7f3c84-d2e1-4b96-8e4d-7c1b9a2f6e05");
    }
}
