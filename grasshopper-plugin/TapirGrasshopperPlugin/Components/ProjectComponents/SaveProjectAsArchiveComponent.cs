using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SaveProjectAsArchiveComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SaveProjectAsArchive";

        public SaveProjectAsArchiveComponent()
            : base(
                "SaveProjectAsArchive",
                "Save the open project as a .pla archive file. An existing file " +
                "is overwritten, and the archive becomes the open project, as " +
                "Save As does.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ArchiveFilePath",
                "Absolute path of the .pla archive file to write. An existing " +
                "file is overwritten.");

            InBoolean(
                "IncludeLibraryParts",
                "Include the library parts the project uses in the archive.",
                true);

            InBoolean(
                "IncludeProperties",
                "Include the properties in the archive.",
                true);

            InBoolean(
                "IncludeTextures",
                "Include the textures in the archive.",
                true);

            InBoolean(
                "IncludeBackgroundPicture",
                "Include the background picture in the archive.",
                true);
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string archiveFilePath = null;
            if (!da.GetData(0, ref archiveFilePath) ||
                string.IsNullOrWhiteSpace(archiveFilePath))
            {
                return;
            }

            var parameters = new JObject
            {
                ["archiveFilePath"] = archiveFilePath,
                ["includeLibraryParts"] = da.GetOptional(1, true),
                ["includeProperties"] = da.GetOptional(2, true),
                ["includeTextures"] = da.GetOptional(3, true),
                ["includeBackgroundPicture"] = da.GetOptional(4, true)
            };

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            da.SetData(0, response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SaveProjectAsArchive;

        public override Guid ComponentGuid =>
            new Guid("3f8a5f21-4c47-4b9d-9c46-71a3c1e0b5d2");
    }
}
