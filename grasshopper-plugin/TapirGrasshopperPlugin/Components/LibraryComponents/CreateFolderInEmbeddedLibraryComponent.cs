using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class CreateFolderInEmbeddedLibraryComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateFolderInEmbeddedLibrary";

        public CreateFolderInEmbeddedLibraryComponent()
            : base(
                "CreateFolderInEmbeddedLibrary",
                "Creates a folder, with any missing parent folders, inside the embedded library. " +
                "The FolderPath is relative to the embedded library, like the OutputPaths of AddFilesToEmbeddedLibrary.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "FolderPath",
                "Path of the folder relative to the embedded library, for example 'Office/Doors'. " +
                "Missing parent folders are created too.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "ErrorMessage",
                "Error message of the command (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string folderPath = null;
            if (!da.GetData(0, ref folderPath))
            {
                return;
            }

            var parameters = new JObject { ["folderPath"] = folderPath };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                LibraryComponentHelp.ExecutionResultMessage(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateFolderInEmbeddedLibrary;

        public override Guid ComponentGuid =>
            new Guid("919b518f-4f46-408f-a65a-fab08d0b785f");
    }
}
