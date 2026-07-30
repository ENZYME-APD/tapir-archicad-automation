using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class ExportFavoritesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ExportFavorites";

        public ExportFavoritesComponent()
            : base(
                "ExportFavorites",
                "Export the project's Favorites to a .prefs file or folder.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Path",
                "Absolute path on the Archicad host. A .prefs extension writes a single file; otherwise it is treated as a folder.");

            InTexts(
                "Names",
                "Optional subset of Favorites to export. Default: export all.");

            SetOptionality(1);
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

            var input = new ExportFavoritesParameters { Path = path };

            if (da.TryGetList(
                    1,
                    out List<string> names) &&
                names.Count > 0)
            {
                input.Names = names;
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ExportFavorites;

        public override Guid ComponentGuid =>
            new Guid("e8515226-f23d-4b5f-86fe-2ab4a13aa1de");
    }
}
