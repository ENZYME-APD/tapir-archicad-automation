using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.CustomData;

namespace TapirGrasshopperPlugin.Components.CustomDataComponents
{
    public class DeleteProjectDataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteProjectData";

        public DeleteProjectDataComponent()
            : base(
                "DeleteProjectData",
                "Delete the project data stored by SetProjectData under the given names. " +
                "Deleting a name that does not exist fails for that name only.",
                GroupNames.CustomData)
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
                "Error message of each name (empty when it was deleted).");
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

            if (names.Exists(string.IsNullOrEmpty))
            {
                this.AddError("Every name must be a non-empty text.");
                return;
            }

            var input = new ProjectDataNamesParameters
            {
                Names = names
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        // The project data has no icon of its own yet; it shares the one of
        // the project info field deletion.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteProjectInfoFields;

        public override Guid ComponentGuid =>
            new Guid("a94d2c7b-1e58-4b3f-86d0-3f7e2c9a6b15");
    }
}
