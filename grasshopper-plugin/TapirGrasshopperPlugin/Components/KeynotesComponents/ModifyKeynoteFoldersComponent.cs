using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Keynotes;

namespace TapirGrasshopperPlugin.Components.KeynotesComponents
{
    public class ModifyKeynoteFoldersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyKeynoteFolders";

        public ModifyKeynoteFoldersComponent()
            : base(
                "ModifyKeynoteFolders",
                "Modify the key, title or reference of the given keynote folders. " +
                "Only provided fields are changed. Available from Archicad 28.",
                GroupNames.Keynotes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "FolderGuids",
                "Identifiers of the keynote folders to modify.");

            InTexts(
                "NewKeys",
                "New key of each folder (input only 1 to use the same key for all). Optional.");

            InTexts(
                "NewTitles",
                "New title of each folder (input only 1 to use the same title for all). Optional.");

            InTexts(
                "NewReferences",
                "New reference of each folder (input only 1 to use the same reference for all). Optional.");

            SetOptionality(new[] { 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each keynote folder (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> folderWrappers))
            {
                return;
            }

            da.TryGetList(1, out List<string> newKeys);
            newKeys = newKeys ?? new List<string>();
            da.TryGetList(2, out List<string> newTitles);
            newTitles = newTitles ?? new List<string>();
            da.TryGetList(3, out List<string> newReferences);
            newReferences = newReferences ?? new List<string>();

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("NewKeys", newKeys.Count),
                         ("NewTitles", newTitles.Count),
                         ("NewReferences", newReferences.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != folderWrappers.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input FolderGuids.");
                    return;
                }
            }

            var items = new JArray();
            for (var i = 0; i < folderWrappers.Count; i++)
            {
                var id = GuidObject<KeynoteFolderGuid>.CreateFromWrapper(folderWrappers[i]);
                if (id == null)
                {
                    this.AddError("Invalid folder identifier in the FolderGuids input.");
                    return;
                }

                var item = new JObject
                {
                    ["keynoteFolderId"] = new JObject { ["guid"] = id.Guid }
                };
                if (newKeys.Count > 0)
                {
                    item["key"] = newKeys[newKeys.Count == 1 ? 0 : i];
                }
                if (newTitles.Count > 0)
                {
                    item["title"] = newTitles[newTitles.Count == 1 ? 0 : i];
                }
                if (newReferences.Count > 0)
                {
                    item["reference"] = newReferences[newReferences.Count == 1 ? 0 : i];
                }
                items.Add(item);
            }

            var parameters = new JObject { ["foldersData"] = items };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyKeynoteFolders;

        public override Guid ComponentGuid =>
            new Guid("341b96fb-87b3-417b-9cdf-8e9eab3efdce");
    }
}
