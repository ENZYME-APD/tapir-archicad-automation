using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class GetClassificationItemVisibilityComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetClassificationItemVisibility";

        public GetClassificationItemVisibilityComponent()
            : base(
                "GetClassificationItemVisibility",
                "Tell whether the given classification items are visible (offered) for the given elements, " +
                "element tool defaults and attributes, such as building materials. Visibility does not tell " +
                "whether the target is classified with the item, and an item can be visible even if its parent " +
                "item is not. The targets are checked in the order ElementGuids, ElementTypes, AttributeGuids; " +
                "the outputs have one branch per target in that order.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements to check the classification items for. Optional.");

            InTexts(
                "ElementTypes",
                "Element types whose tool defaults to check the classification items for. Optional.");

            InGenerics(
                "AttributeGuids",
                "Attributes (for example building materials) to check the classification items for. Optional.");

            InGenerics(
                "ClassificationItemGuids",
                "Identifiers of the classification items to check.");

            SetOptionality(
                new[]
                {
                    0,
                    1,
                    2
                });
        }

        protected override void AddOutputs()
        {
            OutBooleanTree(
                "IsVisible",
                "Whether each classification item is visible for the target (one branch per target, " +
                "one item per classification item; null on error).");

            OutTextTree(
                "VisibilityErrorMessages",
                "Error message for each target and classification item (one branch per target; " +
                "empty when successful).");

            OutErrorMessages(
                "Error message for each target (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var targets = new JArray();

            da.TryGetList(
                0,
                out List<GH_ObjectWrapper> elementWrappers);
            foreach (var wrapper in elementWrappers ?? new List<GH_ObjectWrapper>())
            {
                var id = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid element identifier in the ElementGuids input.");
                    return;
                }
                targets.Add(
                    new JObject
                    {
                        ["elementId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            da.TryGetList(
                1,
                out List<string> elementTypes);
            foreach (var elementType in elementTypes ?? new List<string>())
            {
                targets.Add(new JObject { ["elementType"] = elementType });
            }

            da.TryGetList(
                2,
                out List<GH_ObjectWrapper> attributeWrappers);
            foreach (var wrapper in attributeWrappers ?? new List<GH_ObjectWrapper>())
            {
                var id = GuidObject<AttributeGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }
                targets.Add(
                    new JObject
                    {
                        ["attributeId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            if (targets.Count == 0)
            {
                this.AddError(
                    "Give at least one target in the ElementGuids, ElementTypes or AttributeGuids input.");
                return;
            }

            if (!da.TryGetList(
                    3,
                    out List<GH_ObjectWrapper> itemWrappers))
            {
                return;
            }

            var itemIds = new JArray();
            foreach (var wrapper in itemWrappers)
            {
                var id = GuidObject<ClassificationGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError(
                        "Invalid classification item identifier in the ClassificationItemGuids input.");
                    return;
                }
                itemIds.Add(
                    new JObject
                    {
                        ["classificationItemId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            var parameters = new JObject
            {
                ["targets"] = targets,
                ["classificationItemIds"] = itemIds
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var isVisible = new DataTree<object>();
            var visibilityErrors = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "classificationItemVisibilities");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                isVisible.EnsurePath(path);
                visibilityErrors.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (!(item["visibilities"] is JArray visibilities))
                {
                    continue;
                }

                foreach (var visibility in visibilities)
                {
                    if (JsonOutputHelp.IsError(visibility))
                    {
                        isVisible.Add(null, path);
                        visibilityErrors.Add(
                            JsonOutputHelp.ErrorMessage(visibility),
                            path);
                        continue;
                    }

                    isVisible.Add(
                        JsonOutputHelp.Scalar(
                            visibility,
                            "isVisible"),
                        path);
                    visibilityErrors.Add("", path);
                }
            }

            da.SetDataTree(0, isVisible);
            da.SetDataTree(1, visibilityErrors);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetClassificationItemVisibility;

        public override Guid ComponentGuid =>
            new Guid("e8ece8f1-2078-4ba4-8fb2-f9fe5a90c57f");
    }
}
