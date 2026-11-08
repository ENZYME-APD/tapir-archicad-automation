using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class ChangeFavoriteComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ChangeFavorite";

        public ChangeFavoriteComponent()
            : base(
                "ChangeFavorite",
                "Give an existing Favorite the settings of another Favorite or of the current Opening tool " +
                "default. Available from Archicad 29.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "FavoriteName",
                "Name of the Favorite to change.");

            InText(
                "SourceFavorite",
                "Take the settings of this Favorite. Give either SourceFavorite or ToolDefault.");

            InText(
                "ToolDefault",
                "Take the current settings of this tool (only Opening). Give either SourceFavorite or ToolDefault.");

            SetOptionality(new[] { 1, 2 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string favoriteName) ||
                string.IsNullOrEmpty(favoriteName))
            {
                this.AddError("FavoriteName is required.");
                return;
            }

            if (!FavoriteSource.TryGet(
                    da,
                    1,
                    2,
                    out JObject source,
                    out string sourceError))
            {
                this.AddError(sourceError);
                return;
            }

            var parameters = new JObject
            {
                ["favoriteName"] = favoriteName,
                ["source"] = source
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            if ((bool?)response?["success"] != true)
            {
                this.AddError(
                    response?["error"]?["message"]?.ToString() ??
                    "Failed to change the Favorite.");
            }
        }

        public override Guid ComponentGuid =>
            new Guid("b52d0f6e-81c9-4a37-b4e2-0d6f93a1c728");
    }
}
