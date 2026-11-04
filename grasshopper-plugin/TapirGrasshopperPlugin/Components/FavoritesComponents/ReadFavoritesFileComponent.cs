using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class ReadFavoritesFileComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "ReadFavoritesFile";

        public ReadFavoritesFileComponent()
            : base(
                "ReadFavoritesFile",
                "Read the Favorites of a .prefs file or folder without importing them: the name, element type " +
                "and folder of each. Nothing in the project changes, so the result can be compared with the " +
                "project's Favorites before ImportFavorites.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Path",
                "Absolute path on the Archicad host to a Favorites file (.prefs) or folder.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "FavoriteNames",
                "Name of each Favorite of the file or folder.");

            OutTexts(
                "ElementTypes",
                "Element type of each Favorite.");

            OutTextTree(
                "Folders",
                "Folder hierarchy of each Favorite in the file or folder, outermost folder first " +
                "(one branch per Favorite; empty for the root).");
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

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["path"] = path },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var names = new List<object>();
            var elementTypes = new List<object>();
            var folders = new DataTree<string>();

            var favorites = JsonOutputHelp.Items(
                response,
                "favorites");
            for (var i = 0; i < favorites.Count; i++)
            {
                var favorite = favorites[i];
                var branch = new GH_Path(i);
                names.Add(JsonOutputHelp.Scalar(favorite, "name"));
                elementTypes.Add(JsonOutputHelp.Scalar(favorite, "elementType"));
                folders.EnsurePath(branch);
                if (favorite["folder"] is JArray folder)
                {
                    foreach (var name in folder)
                    {
                        folders.Add(name.ToString(), branch);
                    }
                }
            }

            da.SetDataList(0, names);
            da.SetDataList(1, elementTypes);
            da.SetDataTree(2, folders);
        }

        // Shares the icon of ImportFavorites.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ImportFavorites;

        public override Guid ComponentGuid =>
            new Guid("69369cb9-cedc-44f6-8bbc-ed79d63eb0fe");
    }
}
