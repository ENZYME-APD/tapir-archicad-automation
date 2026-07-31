using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class CreateClassificationItemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateClassificationItems";

        public CreateClassificationItemsComponent()
            : base(
                "CreateClassificationItems",
                "Create Classification Items in the given Classification Systems. " +
                "Child item hierarchies can be given through the ChildrenItems input as JSON.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Ids",
                "User specified unique identifier of each new classification item.");

            InTexts(
                "Names",
                "Display name of each new classification item. Input only 1 to use the same value for all items.");

            InTexts(
                "Descriptions",
                "Description of each new classification item. Input only 1 to use the same value for all items.");

            InGenerics(
                "ClassificationSystemGuids",
                "Identifiers of the classification systems to create the items in. Input only 1 to use the same system for all items.");

            InGenerics(
                "ParentItemGuids",
                "Identifiers of the parent classification items (omit to create the items under the root). Input only 1 to use the same parent for all items. Optional.");

            InTexts(
                "ChildrenItems",
                "One JSON array per item with the child item hierarchy, e.g. " +
                "[{\"id\":\"01\",\"name\":\"...\",\"description\":\"...\",\"children\":[...]}]. " +
                "Input only 1 to use the same children for all items. Optional.");

            SetOptionality(new[] { 4, 5 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "ErrorMessages",
                "Error message for each item (empty when the classification item was created successfully).");
        }

        private static string GetAt(
            List<string> values,
            int index)
        {
            return values[values.Count == 1 ? 0 : index];
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> ids))
            {
                return;
            }

            var itemCount = ids.Count;
            if (itemCount == 0)
            {
                this.AddError("The Ids input must contain at least one item.");
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> names))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<string> descriptions))
            {
                return;
            }

            if (!da.TryGetList(
                    3,
                    out List<GH_ObjectWrapper> systemWrappers))
            {
                return;
            }

            da.TryGetList(
                4,
                out List<GH_ObjectWrapper> parentWrappers);
            parentWrappers = parentWrappers ?? new List<GH_ObjectWrapper>();

            da.TryGetList(
                5,
                out List<string> childrenItems);
            childrenItems = childrenItems ?? new List<string>();

            foreach (var pair in new (string Name, int Count, bool Optional)[]
                     {
                         ("Names", names.Count, false),
                         ("Descriptions", descriptions.Count, false),
                         ("ClassificationSystemGuids", systemWrappers.Count, false),
                         ("ParentItemGuids", parentWrappers.Count, true),
                         ("ChildrenItems", childrenItems.Count, true)
                     })
            {
                var validEmpty = pair.Optional && pair.Count == 0;
                if (!validEmpty && pair.Count != 1 && pair.Count != itemCount)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be {(pair.Optional ? "0, " : "")}1 or equal to the size of the input Ids.");
                    return;
                }
            }

            var items = new JArray();
            for (var i = 0; i < itemCount; i++)
            {
                var systemId = GuidObject<ClassificationGuid>.CreateFromWrapper(
                    systemWrappers[systemWrappers.Count == 1 ? 0 : i]);
                if (systemId == null)
                {
                    this.AddError("Invalid identifier in the ClassificationSystemGuids input.");
                    return;
                }

                var details = new JObject
                {
                    ["id"] = ids[i],
                    ["name"] = GetAt(names, i),
                    ["description"] = GetAt(descriptions, i)
                };

                if (childrenItems.Count > 0)
                {
                    var json = GetAt(childrenItems, i);
                    try
                    {
                        details["children"] = JArray.Parse(json);
                    }
                    catch (Exception ex)
                    {
                        this.AddError(
                            $"Invalid JSON in the ChildrenItems input: {ex.Message}");
                        return;
                    }
                }

                var item = new JObject
                {
                    ["classificationSystemId"] = new JObject { ["guid"] = systemId.Guid },
                    ["classificationItemDetails"] = details
                };

                if (parentWrappers.Count > 0)
                {
                    var parentId = GuidObject<ClassificationGuid>.CreateFromWrapper(
                        parentWrappers[parentWrappers.Count == 1 ? 0 : i]);
                    if (parentId == null)
                    {
                        this.AddError("Invalid identifier in the ParentItemGuids input.");
                        return;
                    }
                    item["parentClassificationItemId"] = new JObject { ["guid"] = parentId.Guid };
                }

                items.Add(item);
            }

            var parameters = new JObject { ["newClassificationItems"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var errors = new List<string>();
            if (response["executionResults"] is JArray results)
            {
                foreach (var result in results)
                {
                    errors.Add(
                        (bool?)result["success"] == true
                            ? ""
                            : result["error"]?["message"]?.ToString() ?? "Unknown error.");
                }
            }
            da.SetDataList(0, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateClassificationItems;

        public override Guid ComponentGuid =>
            new Guid("9bfde227-fc58-4656-a266-40e045a5e80b");
    }
}
