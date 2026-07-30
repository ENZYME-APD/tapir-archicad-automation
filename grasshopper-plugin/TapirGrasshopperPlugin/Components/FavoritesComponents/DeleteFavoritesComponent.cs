using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Favorites;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class DeleteFavoritesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteFavorites";

        public DeleteFavoritesComponent()
            : base(
                "DeleteFavorites",
                "Delete the given Favorites.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "FavoriteNames",
                "Names of the Favorites to delete.");
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

            var input = new DeleteFavoritesParameters
            {
                Favorites = names
            };

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteFavorites;

        public override Guid ComponentGuid =>
            new Guid("1e5e63e1-f601-4416-a3bc-7002780bdb43");
    }
}
