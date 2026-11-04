using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class DeleteFavoriteFolderComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteFavoriteFolder";

        public DeleteFavoriteFolderComponent()
            : base(
                "DeleteFavoriteFolder",
                "Delete a folder of the Favorites palette given by its path. Archicad deletes the folder's " +
                "Favorites and subfolders with it, so a folder that is not empty is deleted only if Recursive is " +
                "true. The root folder cannot be deleted. Available from Archicad 29.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Folder",
                "Folder hierarchy of the folder to delete, outermost folder first (as GetFavoriteFolders " +
                "returns it).");

            InBoolean(
                "Recursive",
                "Also delete a folder that is not empty, with its Favorites and subfolders. Default is false.",
                false);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Folder",
                "Folder hierarchy of the deleted folder, outermost folder first.");

            OutTexts(
                "FavoriteNames",
                "Names of the Favorites that were directly in the deleted folder.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> path) ||
                path.Count == 0)
            {
                this.AddError("Folder is required; the root folder cannot be deleted.");
                return;
            }

            var recursive = da.GetOptional(
                1,
                false);

            var parameters = new JObject
            {
                ["path"] = new JArray(path),
                ["recursive"] = recursive
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var deletedFolder = response["deletedFolder"];
            var favoriteNames = new List<string>();
            if (deletedFolder?["favorites"] is JArray favorites)
            {
                foreach (var name in favorites)
                {
                    favoriteNames.Add(name.ToString());
                }
            }

            da.SetDataList(
                0,
                FavoriteFolderOutputHelp.PathOf(deletedFolder));
            da.SetDataList(
                1,
                favoriteNames);
        }

        // Shares the icon of DeleteFavorites.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteFavorites;

        public override Guid ComponentGuid =>
            new Guid("de9fd857-50c6-4bbd-956d-4a51734e1f9b");
    }
}
