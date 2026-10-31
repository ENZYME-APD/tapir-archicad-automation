using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class DeleteProjectDataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteProjectData";

        public DeleteProjectDataComponent()
            : base(
                "DeleteProjectData",
                "Deletes the project data with the given names.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the project data to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each name (empty when its project data was deleted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ProjectDataHelp.TryGetNames(
                    da,
                    out var parameters))
            {
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteProjectData;

        public override Guid ComponentGuid =>
            new Guid("0fed71fb-91f7-43f8-a40c-dde76a66957a");
    }
}
