using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class DeleteFavoritesWithManagerComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteFavoritesWithManager";

        public DeleteFavoritesWithManagerComponent()
            : base(
                "DeleteFavoritesWithManager",
                "Delete the given Favorites through the Favorite manager, each found by its name, optionally only " +
                "directly in the given folder. Available from Archicad 29.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "FavoriteNames",
                "Names of the Favorites to delete.");

            InTextTree(
                "FolderPaths",
                "Folder hierarchy of the folder each Favorite is in, outermost folder first, as GetFavoriteContents " +
                "returns it (one branch per Favorite, or one branch for all; an empty branch is the root folder). " +
                "A Favorite is then only looked for directly in that folder. Leave it unconnected to find each " +
                "Favorite by its name anywhere. Optional.");

            SetOptionality(1);
        }

        private void InTextTree(
            string name,
            string description)
        {
            inManager.AddTextParameter(
                name,
                name,
                description.WithTypeName("textTree"),
                GH_ParamAccess.tree);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each Favorite (empty when it was deleted).");
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

            da.TryGetTree(1, out GH_Structure<GH_String> folderPaths);
            var folderBranches = folderPaths?.Branches ?? new List<List<GH_String>>();
            if (folderBranches.Count > 1 &&
                folderBranches.Count != names.Count)
            {
                this.AddError(
                    "The FolderPaths input must have 0, 1 or as many branches as the size of the input FavoriteNames.");
                return;
            }

            var favorites = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var favorite = new JObject { ["name"] = names[i] };

                if (folderBranches.Count > 0)
                {
                    var folderPath = new JArray();
                    foreach (var folderName in folderBranches[folderBranches.Count == 1 ? 0 : i])
                    {
                        if (!string.IsNullOrEmpty(folderName?.Value))
                        {
                            folderPath.Add(folderName.Value);
                        }
                    }
                    favorite["folderPath"] = folderPath;
                }

                favorites.Add(favorite);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["favorites"] = favorites },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteFavoritesWithManager;

        public override Guid ComponentGuid =>
            new Guid("65ee4411-9076-4c27-b0e6-e5e89faeb867");
    }
}
