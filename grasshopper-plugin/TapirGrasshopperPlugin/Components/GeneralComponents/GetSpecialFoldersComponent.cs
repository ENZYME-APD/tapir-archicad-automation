using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetSpecialFoldersComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetSpecialFolders";

        public GetSpecialFoldersComponent()
            : base(
                "GetSpecialFolders",
                "Get the filesystem paths of the special folders of the running Archicad.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "FolderTypes",
                "Types of the special folders to retrieve (e.g. UserDocuments, Temporary, EmbeddedProjectLibrary).");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "FolderPaths",
                "Path of each requested special folder (null when the folder is not available).");

            OutTexts(
                "ErrorMessages",
                "Error message for each requested folder (empty on success).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var folderTypes = new List<string>();
            if (!da.GetDataList(
                    0,
                    folderTypes))
            {
                return;
            }

            var input = new JObject
            {
                ["folderTypes"] = JArray.FromObject(folderTypes)
            };

            if (!TryGetCadResponse(
                    CommandName,
                    input,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var paths = new List<string>();
            var errors = new List<string>();

            if (response["folderPaths"] is JArray items)
            {
                foreach (var item in items)
                {
                    if (item?["error"] != null)
                    {
                        errors.Add(item["error"]?["message"]?.ToString() ?? "");
                        paths.Add(null);
                        continue;
                    }

                    errors.Add("");
                    paths.Add(item?["path"]?.ToString());
                }
            }

            da.SetDataList(0, paths);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetSpecialFolders;

        public override Guid ComponentGuid =>
            new Guid("89da90d0-6f37-4747-b803-5090340f572d");
    }
}
