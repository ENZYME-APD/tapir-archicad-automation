using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class MoveFavoriteFolderComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "MoveFavoriteFolder";

        public MoveFavoriteFolderComponent()
            : base(
                "MoveFavoriteFolder",
                "Move a folder of the Favorites palette, with its Favorites and subfolders, into another " +
                "folder. Available from Archicad 29.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Folder",
                "Folder hierarchy of the folder to move, outermost folder first. The root cannot be moved.");

            InTexts(
                "TargetFolder",
                "Folder hierarchy of the folder to move it into, outermost folder first. Empty = root.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Folder",
                "New folder hierarchy of the moved folder, outermost folder first.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> folder) ||
                folder.Count == 0)
            {
                this.AddError("Folder is required.");
                return;
            }

            da.TryGetList(
                1,
                out List<string> targetFolder);

            var parameters = new JObject
            {
                ["folder"] = new JArray(folder),
                ["targetFolder"] = new JArray(targetFolder)
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
                (response["folder"] as JArray)?.Select(x => x.ToString()) ??
                Enumerable.Empty<string>());
        }

        public override Guid ComponentGuid =>
            new Guid("e6f3b710-2a4d-4e98-a1c5-f83b07d6e249");
    }
}
