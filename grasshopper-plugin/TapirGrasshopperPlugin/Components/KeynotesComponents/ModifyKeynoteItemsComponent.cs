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
    public class ModifyKeynoteItemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyKeynoteItems";

        public ModifyKeynoteItemsComponent()
            : base(
                "ModifyKeynoteItems",
                "Modify the key, title, description or reference of the given keynote items. " +
                "Only provided fields are changed. Available from Archicad 28.",
                GroupNames.Keynotes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ItemGuids",
                "Identifiers of the keynote items to modify.");

            InTexts(
                "NewKeys",
                "New key of each item (input only 1 to use the same key for all). Optional.");

            InTexts(
                "NewTitles",
                "New title of each item (input only 1 to use the same title for all). Optional.");

            InTexts(
                "NewDescriptions",
                "New description of each item (input only 1 to use the same description for all). Optional.");

            InTexts(
                "NewReferences",
                "New reference of each item (input only 1 to use the same reference for all). Optional.");

            SetOptionality(new[] { 1, 2, 3, 4 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> itemWrappers))
            {
                return;
            }

            da.TryGetList(1, out List<string> newKeys);
            newKeys = newKeys ?? new List<string>();
            da.TryGetList(2, out List<string> newTitles);
            newTitles = newTitles ?? new List<string>();
            da.TryGetList(3, out List<string> newDescriptions);
            newDescriptions = newDescriptions ?? new List<string>();
            da.TryGetList(4, out List<string> newReferences);
            newReferences = newReferences ?? new List<string>();

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("NewKeys", newKeys.Count),
                         ("NewTitles", newTitles.Count),
                         ("NewDescriptions", newDescriptions.Count),
                         ("NewReferences", newReferences.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != itemWrappers.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input ItemGuids.");
                    return;
                }
            }

            var items = new JArray();
            for (var i = 0; i < itemWrappers.Count; i++)
            {
                var id = GuidObject<KeynoteItemGuid>.CreateFromWrapper(itemWrappers[i]);
                if (id == null)
                {
                    this.AddError("Invalid item identifier in the ItemGuids input.");
                    return;
                }

                var item = new JObject
                {
                    ["keynoteItemId"] = new JObject { ["guid"] = id.Guid }
                };
                if (newKeys.Count > 0)
                {
                    item["key"] = newKeys[newKeys.Count == 1 ? 0 : i];
                }
                if (newTitles.Count > 0)
                {
                    item["title"] = newTitles[newTitles.Count == 1 ? 0 : i];
                }
                if (newDescriptions.Count > 0)
                {
                    item["description"] = newDescriptions[newDescriptions.Count == 1 ? 0 : i];
                }
                if (newReferences.Count > 0)
                {
                    item["reference"] = newReferences[newReferences.Count == 1 ? 0 : i];
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
            Properties.Resources.ModifyKeynoteItems;

        public override Guid ComponentGuid =>
            new Guid("72277d23-a61a-4f87-bd14-b3eec5dbf75a");
    }
}
