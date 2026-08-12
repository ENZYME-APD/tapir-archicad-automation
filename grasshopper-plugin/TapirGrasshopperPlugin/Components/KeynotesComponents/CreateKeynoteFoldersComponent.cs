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
    public class CreateKeynoteFoldersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateKeynoteFolders";

        public CreateKeynoteFoldersComponent()
            : base(
                "CreateKeynoteFolders",
                "Create keynote folders under the given parent folders (or under the root folder). " +
                "Available from Archicad 28.",
                GroupNames.Keynotes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Keys",
                "Key of each keynote folder to create.");

            InTexts(
                "Titles",
                "Title of each keynote folder to create (input only 1 to use the same title for all).");

            InGenerics(
                "ParentFolderGuids",
                "Identifier of the parent folder of each new folder (input only 1 to use the same parent for all). " +
                "Optional; defaults to the root folder.");

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "KeynoteFolderGuids",
                "Identifier of each created keynote folder.");

            OutErrorMessages(
                "Error message of each folder (empty when it was created successfully).");
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

            if (!da.TryGetList(
                    1,
                    out List<string> titles))
            {
                return;
            }

            if (titles.Count != 1 &&
                titles.Count != keys.Count)
            {
                this.AddError(
                    "The size of the input Titles must be 1 or equal to the size of the input Keys.");
                return;
            }

            da.TryGetList(
                2,
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
                var item = new JObject
                {
                    ["key"] = keys[i],
                    ["title"] = titles[titles.Count == 1 ? 0 : i]
                };
                if (parentIds.Count > 0)
                {
                    item["parentFolderId"] = new JObject
                    {
                        ["guid"] = parentIds[parentIds.Count == 1 ? 0 : i].Guid
                    };
                }
                items.Add(item);
            }

            var parameters = new JObject { ["foldersData"] = items };

            SetCadValuesWithCreatedIds<KeynoteFolderGuid>(
                CommandName,
                parameters,
                ToAddOn,
                da,
                "keynoteFolderIdsOrErrors",
                "keynoteFolderId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateKeynoteFolders;

        public override Guid ComponentGuid =>
            new Guid("baff3035-df0b-4f0d-8220-8c9b04be58d4");
    }
}
