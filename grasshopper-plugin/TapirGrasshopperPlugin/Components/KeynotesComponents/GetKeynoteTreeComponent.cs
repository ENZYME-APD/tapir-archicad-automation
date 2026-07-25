using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Keynotes;

namespace TapirGrasshopperPlugin.Components.KeynotesComponents
{
    public class GetKeynoteTreeComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetKeynoteTree";

        public GetKeynoteTreeComponent()
            : base(
                "GetKeynoteTree",
                "Get the whole keynote folder and item hierarchy. " +
                "The folder outputs include the root folder (with a null parent). " +
                "Available from Archicad 28.",
                GroupNames.Keynotes)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "FolderGuids",
                "Identifier of each keynote folder (including the root folder).");

            OutTexts(
                "FolderKeys",
                "Key of each keynote folder.");

            OutTexts(
                "FolderTitles",
                "Title of each keynote folder.");

            OutTexts(
                "FolderReferences",
                "Reference of each keynote folder.");

            OutGenerics(
                "FolderParentGuids",
                "Identifier of the parent of each keynote folder (null for the root folder).");

            OutGenerics(
                "ItemGuids",
                "Identifier of each keynote item.");

            OutTexts(
                "ItemKeys",
                "Key of each keynote item.");

            OutTexts(
                "ItemTitles",
                "Title of each keynote item.");

            OutTexts(
                "ItemDescriptions",
                "Description of each keynote item.");

            OutTexts(
                "ItemReferences",
                "Reference of each keynote item.");

            OutGenerics(
                "ItemFolderGuids",
                "Identifier of the folder containing each keynote item.");
        }

        private static KeynoteFolderGuidWrapper FolderIdOf(
            JToken folder)
        {
            var guid = folder?["keynoteFolderId"]?["guid"]?.ToString();
            if (guid == null)
            {
                return null;
            }
            return new KeynoteFolderGuidWrapper
            {
                KeynoteFolderId = new KeynoteFolderGuid { Guid = guid }
            };
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

            var folderGuids = new List<object>();
            var folderKeys = new List<object>();
            var folderTitles = new List<object>();
            var folderReferences = new List<object>();
            var folderParentGuids = new List<object>();
            var itemGuids = new List<object>();
            var itemKeys = new List<object>();
            var itemTitles = new List<object>();
            var itemDescriptions = new List<object>();
            var itemReferences = new List<object>();
            var itemFolderGuids = new List<object>();

            void WalkFolder(
                JToken folder,
                KeynoteFolderGuidWrapper parentId)
            {
                var folderId = FolderIdOf(folder);
                folderGuids.Add(folderId);
                folderKeys.Add(folder["key"]?.ToString());
                folderTitles.Add(folder["title"]?.ToString());
                folderReferences.Add(folder["reference"]?.ToString());
                folderParentGuids.Add(parentId);

                if (folder["items"] is JArray items)
                {
                    foreach (var item in items)
                    {
                        var itemGuid = item["keynoteItemId"]?["guid"]?.ToString();
                        itemGuids.Add(
                            itemGuid == null
                                ? null
                                : new KeynoteItemGuidWrapper
                                {
                                    KeynoteItemId = new KeynoteItemGuid { Guid = itemGuid }
                                });
                        itemKeys.Add(item["key"]?.ToString());
                        itemTitles.Add(item["title"]?.ToString());
                        itemDescriptions.Add(item["description"]?.ToString());
                        itemReferences.Add(item["reference"]?.ToString());
                        itemFolderGuids.Add(folderId);
                    }
                }

                if (folder["subFolders"] is JArray subFolders)
                {
                    foreach (var subFolder in subFolders)
                    {
                        WalkFolder(subFolder, folderId);
                    }
                }
            }

            var rootFolder = response["rootFolder"];
            if (rootFolder == null)
            {
                this.AddError("The response contains no root folder.");
                return;
            }
            WalkFolder(rootFolder, null);

            da.SetDataList(0, folderGuids);
            da.SetDataList(1, folderKeys);
            da.SetDataList(2, folderTitles);
            da.SetDataList(3, folderReferences);
            da.SetDataList(4, folderParentGuids);
            da.SetDataList(5, itemGuids);
            da.SetDataList(6, itemKeys);
            da.SetDataList(7, itemTitles);
            da.SetDataList(8, itemDescriptions);
            da.SetDataList(9, itemReferences);
            da.SetDataList(10, itemFolderGuids);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetKeynoteTree;

        public override Guid ComponentGuid =>
            new Guid("f000b02a-2cf2-4baf-b907-8c8c2a5a7571");
    }
}
