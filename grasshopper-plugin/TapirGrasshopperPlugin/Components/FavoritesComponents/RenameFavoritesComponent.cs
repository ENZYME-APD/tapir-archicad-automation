using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Favorites;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class RenameFavoritesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameFavorites";

        public RenameFavoritesComponent()
            : base(
                "RenameFavorites",
                "Rename the given Favorites.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "OldNames",
                "Current names of the Favorites to rename.");

            InTexts(
                "NewNames",
                "New names of the Favorites.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> oldNames))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> newNames))
            {
                return;
            }

            if (oldNames.Count != newNames.Count)
            {
                this.AddError(
                    "The size of the inputs OldNames and NewNames must be equal.");
                return;
            }

            var input = new RenameFavoritesParameters
            {
                Renames = new List<FavoriteRename>()
            };

            for (var i = 0; i < oldNames.Count; i++)
            {
                input.Renames.Add(
                    new FavoriteRename
                    {
                        OldName = oldNames[i],
                        NewName = newNames[i]
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameFavorites;

        public override Guid ComponentGuid =>
            new Guid("e707bb54-5afe-4b06-aab6-1a2ba41ab66d");
    }
}
