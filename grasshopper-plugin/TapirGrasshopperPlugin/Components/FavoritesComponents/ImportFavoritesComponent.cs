using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class ImportFavoritesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ImportFavorites";

        public ImportFavoritesComponent()
            : base(
                "ImportFavorites",
                "Import Favorites from a .prefs file or folder into the current project.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Path",
                "Absolute path on the Archicad host to a Favorites file (.prefs) or folder.");

            InTexts(
                "TargetFolder",
                "Folder hierarchy under which to import. Empty = root.");

            InBoolean(
                "ImportFolders",
                "If true and Path is a folder, the folder structure is preserved.",
                false);

            InText(
                "ConflictPolicy",
                "How to resolve name conflicts: Error, Skip, Overwrite or Append. Default Overwrite.");

            SetOptionality(new[] { 1, 2, 3 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string path) ||
                string.IsNullOrEmpty(path))
            {
                this.AddError("Path is required.");
                return;
            }

            var input = new ImportFavoritesParameters { Path = path };

            if (da.TryGetList(
                    1,
                    out List<string> targetFolder) &&
                targetFolder.Count > 0)
            {
                input.TargetFolder = targetFolder;
            }

            if (da.TryGet(
                    2,
                    out bool importFolders))
            {
                input.ImportFolders = importFolders;
            }

            if (da.TryGet(
                    3,
                    out string conflictPolicy) &&
                !string.IsNullOrEmpty(conflictPolicy))
            {
                input.ConflictPolicy = conflictPolicy;
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ImportFavorites;

        public override Guid ComponentGuid =>
            new Guid("71428af9-2f30-465e-aa29-29f587ab27de");
    }
}
