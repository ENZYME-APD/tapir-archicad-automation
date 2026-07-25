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
    public class CreateKeynoteItemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateKeynoteItems";

        public CreateKeynoteItemsComponent()
            : base(
                "CreateKeynoteItems",
                "Create keynote items in the given parent folders (or in the root folder). " +
                "Available from Archicad 28.",
                GroupNames.Keynotes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Keys",
                "Key of each keynote item to create.");

            InTexts(
                "Titles",
                "Title of each keynote item (input only 1 to use the same title for all). Optional.");

            InTexts(
                "Descriptions",
                "Description of each keynote item (input only 1 to use the same description for all). Optional.");

            InTexts(
                "References",
                "Reference of each keynote item (input only 1 to use the same reference for all). Optional.");

            InGenerics(
                "ParentFolderGuids",
                "Identifier of the parent folder of each new item (input only 1 to use the same parent for all). " +
                "Optional; defaults to the root folder.");

            SetOptionality(new[] { 1, 2, 3, 4 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> keys))
            {
                return;
            }

            da.TryGetList(1, out List<string> titles);
            titles = titles ?? new List<string>();
            da.TryGetList(2, out List<string> descriptions);
            descriptions = descriptions ?? new List<string>();
            da.TryGetList(3, out List<string> references);
            references = references ?? new List<string>();

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("Titles", titles.Count),
                         ("Descriptions", descriptions.Count),
                         ("References", references.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != keys.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input Keys.");
                    return;
                }
            }

            da.TryGetList(
                4,
                out List<GH_ObjectWrapper> parentWrappers);
            parentWrappers = parentWrappers ?? new List<GH_ObjectWrapper>();
            if (parentWrappers.Count > 1 &&
                parentWrappers.Count != keys.Count)
            {
                this.AddError(
                    "The size of the input ParentFolderGuids must be 0, 1 or equal to the size of the input Keys.");
                return;
            }

            var parentIds = new List<KeynoteFolderGuid>();
            foreach (var wrapper in parentWrappers)
            {
                var id = GuidObject<KeynoteFolderGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid folder identifier in the ParentFolderGuids input.");
                    return;
                }
                parentIds.Add(id);
            }

            var items = new JArray();
            for (var i = 0; i < keys.Count; i++)
            {
                var item = new JObject { ["key"] = keys[i] };
                if (titles.Count > 0)
                {
                    item["title"] = titles[titles.Count == 1 ? 0 : i];
                }
                if (descriptions.Count > 0)
                {
                    item["description"] = descriptions[descriptions.Count == 1 ? 0 : i];
                }
                if (references.Count > 0)
                {
                    item["reference"] = references[references.Count == 1 ? 0 : i];
                }
                if (parentIds.Count > 0)
                {
                    item["parentFolderId"] = new JObject
                    {
                        ["guid"] = parentIds[parentIds.Count == 1 ? 0 : i].Guid
                    };
                }
                items.Add(item);
            }

            var parameters = new JObject { ["itemsData"] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateKeynoteItems;

        public override Guid ComponentGuid =>
            new Guid("cd51e0d4-0be3-490c-b039-d70d683db137");
    }
}
