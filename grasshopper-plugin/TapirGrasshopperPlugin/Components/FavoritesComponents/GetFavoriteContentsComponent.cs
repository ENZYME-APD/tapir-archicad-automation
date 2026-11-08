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
    public class GetFavoriteContentsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetFavoriteContents";

        public GetFavoriteContentsComponent()
            : base(
                "GetFavoriteContents",
                "Get the Favorites of a folder of the Favorites palette and of its subfolders (every Favorite when " +
                "no folder is given), each with its folder path, classifications, element category values and " +
                "property values, read through the Favorite manager. Tree outputs have one branch per Favorite. " +
                "Available from Archicad 29.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "FolderPath",
                "Folder hierarchy of the folder to read, outermost folder first (as GetFavoriteFolders returns it). " +
                "The Favorites of its subfolders are read too. Empty = every Favorite.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "FavoriteNames",
                "Name of each Favorite (null when it could not be read).");

            OutTextTree(
                "FolderPaths",
                "Names of the folders leading from the root to the folder of each Favorite, as Archicad gives " +
                "them (empty for the root folder). DeleteFavoritesWithManager takes them as its FolderPaths.");

            OutGenericTree(
                "ClassificationSystemGuids",
                "Classification system of each classification stored in the Favorite.");

            OutGenericTree(
                "ClassificationItemGuids",
                "Classification item of each classification stored in the Favorite (null when unclassified).");

            OutTextTree(
                "CategoryTypes",
                "Type of each element category stored in the Favorite: StructuralFunction, Position, " +
                "RenovationStatus, RenovationFilter or BRI.");

            OutTextTree(
                "CategoryValueNames",
                "Localized name of the value of each element category, for display only.");

            OutTextTree(
                "CategoryGuids",
                "Guid of each element category.");

            OutTextTree(
                "CategoryValueGuids",
                "Guid of the value of each element category.");

            OutGenericTree(
                "PropertyGuids",
                "Identifier of each property stored in the Favorite.");

            OutTextTree(
                "PropertyValues",
                "Value of each property as text (null when it could not be read).");

            OutTextTree(
                "PropertyErrorMessages",
                "Error message of each property value (empty when it was read).");

            OutErrorMessages(
                "Error message for each Favorite (empty when it could be read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var folderPath = da.OptionalList<string>(0);

            var parameters = new JObject();
            if (folderPath.Count > 0)
            {
                parameters["folderPath"] = new JArray(folderPath);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var favoriteNames = new List<object>();
            var folderPaths = new DataTree<object>();
            var systemGuids = new DataTree<object>();
            var itemGuids = new DataTree<object>();
            var categoryTypes = new DataTree<object>();
            var categoryValueNames = new DataTree<object>();
            var categoryGuids = new DataTree<object>();
            var categoryValueGuids = new DataTree<object>();
            var propertyGuids = new DataTree<object>();
            var propertyValues = new DataTree<object>();
            var propertyErrors = new DataTree<object>();
            var errors = new List<string>();

            var trees = new[]
            {
                folderPaths,
                systemGuids,
                itemGuids,
                categoryTypes,
                categoryValueNames,
                categoryGuids,
                categoryValueGuids,
                propertyGuids,
                propertyValues,
                propertyErrors
            };

            var items = JsonOutputHelp.Items(
                response,
                "favorites");
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
                    favoriteNames.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                favoriteNames.Add(JsonOutputHelp.Scalar(item, "name"));
                errors.Add("");

                if (item["folderPath"] is JArray itemFolderPath)
                {
                    foreach (var folderName in itemFolderPath)
                    {
                        folderPaths.Add(folderName.ToString(), path);
                    }
                }

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

                if (item["categoryValues"] is JArray categoryValues)
                {
                    foreach (var categoryValue in categoryValues)
                    {
                        categoryTypes.Add(
                            JsonOutputHelp.Scalar(categoryValue, "type"),
                            path);
                        categoryValueNames.Add(
                            JsonOutputHelp.Scalar(categoryValue, "name"),
                            path);
                        categoryGuids.Add(
                            JsonOutputHelp.WrappedGuidOf(categoryValue, "categoryId"),
                            path);
                        categoryValueGuids.Add(
                            JsonOutputHelp.WrappedGuidOf(categoryValue, "categoryValueId"),
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
                        propertyValues.Add(
                            JsonOutputHelp.Scalar(property["propertyValue"], "value"),
                            path);
                        propertyErrors.Add(
                            JsonOutputHelp.ErrorMessage(property),
                            path);
                    }
                }
            }

            da.SetDataList(0, favoriteNames);
            for (var t = 0; t < trees.Length; t++)
            {
                da.SetDataTree(t + 1, trees[t]);
            }
            da.SetDataList(11, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetFavoriteContents;

        public override Guid ComponentGuid =>
            new Guid("61d3827c-a658-449a-a433-61958edadc6a");
    }
}
