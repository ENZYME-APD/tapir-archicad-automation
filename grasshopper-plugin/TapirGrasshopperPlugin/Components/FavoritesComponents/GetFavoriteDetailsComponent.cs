using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class GetFavoriteDetailsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetFavoriteDetails";

        public GetFavoriteDetailsComponent()
            : base(
                "GetFavoriteDetails",
                "Get the classifications, property values and element category values stored in the given " +
                "favorites, so they can be audited before elements are placed. Each property value comes with its " +
                "evaluation status and default-value flag; the value itself is only returned when it has been " +
                "evaluated. Tree outputs have one branch per favorite.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "FavoriteNames",
                "Names of the favorites.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "ElementTypes",
                "Element type of each favorite, as in GetFavoritesByType.");

            OutGenericTree(
                "ClassificationSystemGuids",
                "Classification system of each classification the favorite applies.");

            OutGenericTree(
                "ClassificationItemGuids",
                "Classification item of each classification the favorite applies (null when unclassified).");

            OutGenericTree(
                "PropertyGuids",
                "Identifier of each property stored in the favorite.");

            OutTextTree(
                "PropertyNames",
                "Name of each property stored in the favorite.");

            OutTextTree(
                "PropertyStatuses",
                "Evaluation status of each property value: HasValue, NotEvaluated or NotAvailable.");

            OutBooleanTree(
                "PropertyIsDefault",
                "True for each property whose value is the default value of the property.");

            OutTextTree(
                "PropertyValues",
                "Value of each property (null unless its status is HasValue).");

            OutTextTree(
                "PropertyErrorMessages",
                "Error message of each property value (empty when there is none).");

            OutTextTree(
                "CategoryNames",
                "Name of each element category (e.g. Structural Function, Position, Renovation Status).");

            OutTextTree(
                "CategoryValueNames",
                "Name of the value of each element category stored in the favorite.");

            OutTextTree(
                "CategoryGuids",
                "Guid of each element category.");

            OutTextTree(
                "CategoryValueGuids",
                "Guid of the value of each element category.");

            OutErrorMessages(
                "Error message for each favorite (empty when it could be read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> favoriteNames))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["favorites"] = JArray.FromObject(favoriteNames) },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elementTypes = new List<object>();
            var systemGuids = new DataTree<object>();
            var itemGuids = new DataTree<object>();
            var propertyGuids = new DataTree<object>();
            var propertyNames = new DataTree<object>();
            var propertyStatuses = new DataTree<object>();
            var propertyIsDefault = new DataTree<object>();
            var propertyValues = new DataTree<object>();
            var propertyErrors = new DataTree<object>();
            var categoryNames = new DataTree<object>();
            var categoryValueNames = new DataTree<object>();
            var categoryGuids = new DataTree<object>();
            var categoryValueGuids = new DataTree<object>();
            var errors = new List<string>();

            var trees = new[]
            {
                systemGuids,
                itemGuids,
                propertyGuids,
                propertyNames,
                propertyStatuses,
                propertyIsDefault,
                propertyValues,
                propertyErrors,
                categoryNames,
                categoryValueNames,
                categoryGuids,
                categoryValueGuids
            };

            var items = JsonOutputHelp.Items(
                response,
                "favoriteDetails");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                foreach (var tree in trees)
                {
                    tree.EnsurePath(path);
                }

                if (JsonOutputHelp.IsError(item))
                {
                    elementTypes.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                elementTypes.Add(JsonOutputHelp.Scalar(item, "elementType"));
                errors.Add("");

                if (item["classifications"] is JArray classifications)
                {
                    foreach (var classification in classifications)
                    {
                        var classificationId = classification["classificationId"];
                        var systemGuid = JsonOutputHelp.WrappedGuidOf(
                            classificationId,
                            "classificationSystemId");
                        var itemGuid = JsonOutputHelp.WrappedGuidOf(
                            classificationId,
                            "classificationItemId");
                        systemGuids.Add(
                            systemGuid == null
                                ? null
                                : new ClassificationSystemGuidWrapper
                                {
                                    ClassificationSystemId =
                                        new ClassificationGuid { Guid = systemGuid }
                                },
                            path);
                        itemGuids.Add(
                            itemGuid == null
                                ? null
                                : new ClassificationItemGuidWrapper
                                {
                                    ClassificationItemId =
                                        new ClassificationGuid { Guid = itemGuid }
                                },
                            path);
                    }
                }

                if (item["properties"] is JArray properties)
                {
                    foreach (var property in properties)
                    {
                        propertyGuids.Add(
                            PropertyGuidObject.FromString(
                                JsonOutputHelp.WrappedGuidOf(
                                    property,
                                    "propertyId")),
                            path);
                        propertyNames.Add(
                            JsonOutputHelp.Scalar(property, "propertyName"),
                            path);
                        propertyStatuses.Add(
                            JsonOutputHelp.Scalar(property, "status"),
                            path);
                        propertyIsDefault.Add(
                            JsonOutputHelp.Scalar(property, "isDefault"),
                            path);
                        propertyValues.Add(
                            JsonOutputHelp.Scalar(property["propertyValue"], "value"),
                            path);
                        propertyErrors.Add(
                            JsonOutputHelp.ErrorMessage(property),
                            path);
                    }
                }

                if (item["categoryValues"] is JArray categoryValues)
                {
                    foreach (var categoryValue in categoryValues)
                    {
                        categoryNames.Add(
                            JsonOutputHelp.Scalar(categoryValue, "categoryName"),
                            path);
                        categoryValueNames.Add(
                            JsonOutputHelp.Scalar(categoryValue, "categoryValueName"),
                            path);
                        categoryGuids.Add(
                            JsonOutputHelp.WrappedGuidOf(categoryValue, "categoryId"),
                            path);
                        categoryValueGuids.Add(
                            JsonOutputHelp.WrappedGuidOf(categoryValue, "categoryValueId"),
                            path);
                    }
                }
            }

            da.SetDataList(0, elementTypes);
            for (var t = 0; t < trees.Length; t++)
            {
                da.SetDataTree(t + 1, trees[t]);
            }
            da.SetDataList(13, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetFavoriteDetails;

        public override Guid ComponentGuid =>
            new Guid("e8fa89f0-2067-4121-ae27-0996e2b55552");
    }
}
