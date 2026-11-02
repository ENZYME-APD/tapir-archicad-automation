using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class GetFavoriteFoldersComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetFavoriteFolders";

        public GetFavoriteFoldersComponent()
            : base(
                "GetFavoriteFolders",
                "Get the folder tree of the Favorites palette from its root folder: the name and path of every " +
                "folder and the names of the Favorites in it. The root folder comes first, then every folder after " +
                "its parent. Available from Archicad 29.",
                GroupNames.Favorites)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "FolderNames",
                "Name of each folder (empty for the root folder).");

            OutTextTree(
                "FolderPaths",
                "Names of the folders leading from the root to each folder, its own name last " +
                "(one branch per folder; empty for the root folder).");

            OutTextTree(
                "FavoriteNames",
                "Names of the Favorites directly in each folder (one branch per folder).");

            OutIntegers(
                "ParentFolderIndices",
                "Index of the parent of each folder in these outputs (-1 for the root folder).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var folderNames = new List<object>();
            var folderPaths = new DataTree<object>();
            var favoriteNames = new DataTree<object>();
            var parentIndices = new List<int>();

            void Walk(
                JToken folder,
                int parentIndex)
            {
                var index = folderNames.Count;
                var path = new GH_Path(index);
                folderNames.Add(JsonOutputHelp.Scalar(folder, "name"));
                parentIndices.Add(parentIndex);
                folderPaths.EnsurePath(path);
                favoriteNames.EnsurePath(path);

                if (folder["path"] is JArray folderPath)
                {
                    foreach (var name in folderPath)
                    {
                        folderPaths.Add(name.ToString(), path);
                    }
                }

                if (folder["favorites"] is JArray favorites)
                {
                    foreach (var name in favorites)
                    {
                        favoriteNames.Add(name.ToString(), path);
                    }
                }

                if (folder["subFolders"] is JArray subFolders)
                {
                    foreach (var subFolder in subFolders)
                    {
                        Walk(subFolder, index);
                    }
                }
            }

            if (response["rootFolder"] is JObject rootFolder)
            {
                Walk(rootFolder, -1);
            }

            da.SetDataList(0, folderNames);
            da.SetDataTree(1, folderPaths);
            da.SetDataTree(2, favoriteNames);
            da.SetDataList(3, parentIndices);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetFavoriteFolders;

        public override Guid ComponentGuid =>
            new Guid("1868b03e-bac6-4fde-8e8f-eb541fc95dad");
    }
}
