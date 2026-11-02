using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class CreateFavoriteComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateFavorite";

        public CreateFavoriteComponent()
            : base(
                "CreateFavorite",
                "Create a Favorite from the settings of another Favorite or of the current Opening tool " +
                "default. Available from Archicad 29.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name of the new Favorite.");

            InTexts(
                "Folder",
                "Folder hierarchy in the Favorites palette to create the Favorite in, outermost folder " +
                "first. The folder must exist. Empty = root.");

            InText(
                "SourceFavorite",
                "Take the settings of this Favorite. Give either SourceFavorite or ToolDefault.");

            InText(
                "ToolDefault",
                "Take the current settings of this tool (only Opening). Give either SourceFavorite or ToolDefault.");

            InText(
                "ConflictPolicy",
                "What to do when a Favorite with the name exists: Error, Skip, OverwriteExisting or " +
                "RenameNew. Default Error.");

            SetOptionality(new[] { 1, 2, 3, 4 });
        }

        protected override void AddOutputs()
        {
            OutText(
                "Name",
                "Name of the Favorite: the given name, or the new name with RenameNew.");

            OutTexts(
                "Folder",
                "Folder hierarchy of the Favorite, outermost folder first. Empty for the root.");

            OutText(
                "Outcome",
                "Created, Skipped or Overwritten.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string name) ||
                string.IsNullOrEmpty(name))
            {
                this.AddError("Name is required.");
                return;
            }

            if (!FavoriteSource.TryGet(
                    da,
                    2,
                    3,
                    out JObject source,
                    out string sourceError))
            {
                this.AddError(sourceError);
                return;
            }

            var parameters = new JObject
            {
                ["name"] = name,
                ["source"] = source
            };

            if (da.TryGetList(
                    1,
                    out List<string> folder) &&
                folder.Count > 0)
            {
                parameters["folder"] = new JArray(folder);
            }

            if (da.TryGet(
                    4,
                    out string conflictPolicy) &&
                !string.IsNullOrEmpty(conflictPolicy))
            {
                parameters["conflictPolicy"] = conflictPolicy;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response["name"]?.ToString() ?? "");
            da.SetDataList(
                1,
                (response["folder"] as JArray)?.Select(x => x.ToString()) ??
                Enumerable.Empty<string>());
            da.SetData(
                2,
                response["outcome"]?.ToString() ?? "");
        }

        public override Guid ComponentGuid =>
            new Guid("3c7e91a4-5d28-4f0b-9e61-7a2c4b8d0f15");
    }

    // The source input of CreateFavorite and ChangeFavorite: the settings of
    // another Favorite or of a tool default, exactly one of them.
    internal static class FavoriteSource
    {
        public static bool TryGet(
            IGH_DataAccess da,
            int favoriteIndex,
            int toolDefaultIndex,
            out JObject source,
            out string error)
        {
            da.TryGet(
                favoriteIndex,
                out string favoriteName);
            da.TryGet(
                toolDefaultIndex,
                out string toolDefault);

            source = null;
            error = null;
            if (string.IsNullOrEmpty(favoriteName) ==
                string.IsNullOrEmpty(toolDefault))
            {
                error = "Give exactly one of SourceFavorite and ToolDefault.";
                return false;
            }

            source = string.IsNullOrEmpty(favoriteName)
                ? new JObject { ["toolDefault"] = toolDefault }
                : new JObject { ["favoriteName"] = favoriteName };
            return true;
        }
    }
}
