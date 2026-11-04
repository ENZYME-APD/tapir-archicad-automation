using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class MoveFavoritesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "MoveFavorites";

        public MoveFavoritesComponent()
            : base(
                "MoveFavorites",
                "Move Favorites given by their names into a folder of the Favorites palette given by its path. " +
                "Available from Archicad 29.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "FavoriteNames",
                "Names of the Favorites to move.");

            InTexts(
                "TargetFolder",
                "Folder hierarchy of the folder to move them into, outermost folder first (as " +
                "GetFavoriteFolders returns it). Empty = root.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each Favorite (empty when it was moved).");
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

            var targetPath = da.OptionalList<string>(1);

            var parameters = new JObject
            {
                ["favorites"] = new JArray(names),
                ["targetPath"] = new JArray(targetPath)
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        // Shares the icon of GetFavoriteFolders.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetFavoriteFolders;

        public override Guid ComponentGuid =>
            new Guid("6d0bb173-6990-4572-90df-59d587ed08f3");
    }
}
