using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Keynotes;

namespace TapirGrasshopperPlugin.Components.KeynotesComponents
{
    public class DeleteKeynoteFoldersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteKeynoteFolders";

        public DeleteKeynoteFoldersComponent()
            : base(
                "DeleteKeynoteFolders",
                "Delete the given keynote folders including their content. " +
                "Available from Archicad 28.",
                GroupNames.Keynotes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "FolderGuids",
                "Identifiers of the keynote folders to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each keynote folder (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out KeynoteFolderIdsObject input))
            {
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteKeynoteFolders;

        public override Guid ComponentGuid =>
            new Guid("3fe14e0f-fca0-4239-97c2-03bac313b3f0");
    }
}
