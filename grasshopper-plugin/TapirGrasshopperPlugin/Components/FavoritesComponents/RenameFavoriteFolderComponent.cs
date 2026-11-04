using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class RenameFavoriteFolderComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameFavoriteFolder";

        public RenameFavoriteFolderComponent()
            : base(
                "RenameFavoriteFolder",
                "Rename a folder of the Favorites palette given by its path. The root folder cannot be renamed. " +
                "Available from Archicad 29.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Folder",
                "Folder hierarchy of the folder to rename, outermost folder first (as GetFavoriteFolders " +
                "returns it).");

            InText(
                "NewName",
                "New name of the folder.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Folder",
                "Folder hierarchy of the renamed folder, outermost folder first.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> path) ||
                path.Count == 0)
            {
                this.AddError("Folder is required; the root folder cannot be renamed.");
                return;
            }

            if (!da.TryGet(
                    1,
                    out string newName) ||
                string.IsNullOrEmpty(newName))
            {
                this.AddError("NewName is required.");
                return;
            }

            var parameters = new JObject
            {
                ["path"] = new JArray(path),
                ["newName"] = newName
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetDataList(
                0,
                FavoriteFolderOutputHelp.PathOf(response["folder"]));
        }

        // Shares the icon of RenameFavorites.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameFavorites;

        public override Guid ComponentGuid =>
            new Guid("2ba392d2-5d2c-4a9d-9823-9ceed20b59e8");
    }
}
