using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class CreateFavoriteFolderComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateFavoriteFolder";

        public CreateFavoriteFolderComponent()
            : base(
                "CreateFavoriteFolder",
                "Create a folder in the Favorites palette, in the parent folder given by its path. " +
                "Available from Archicad 29.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ParentFolder",
                "Folder hierarchy of the folder to create the new folder in, outermost folder first " +
                "(as GetFavoriteFolders returns it). Empty = root.");

            InText(
                "Name",
                "Name of the new folder.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Folder",
                "Folder hierarchy of the new folder, outermost folder first.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parentPath = da.OptionalList<string>(0);

            if (!da.TryGet(
                    1,
                    out string name) ||
                string.IsNullOrEmpty(name))
            {
                this.AddError("Name is required.");
                return;
            }

            var parameters = new JObject
            {
                ["parentPath"] = new JArray(parentPath),
                ["name"] = name
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

        // Shares the icon of GetFavoriteFolders.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetFavoriteFolders;

        public override Guid ComponentGuid =>
            new Guid("106553c4-a632-42bc-9457-7a47f422151d");
    }

    // The folder hierarchy of a FavoriteFolderDetails object of the
    // Favorite folder commands.
    public static class FavoriteFolderOutputHelp
    {
        public static List<string> PathOf(
            JToken folder)
        {
            var path = new List<string>();
            if (folder?["path"] is JArray names)
            {
                foreach (var name in names)
                {
                    path.Add(name.ToString());
                }
            }
            return path;
        }
    }
}
