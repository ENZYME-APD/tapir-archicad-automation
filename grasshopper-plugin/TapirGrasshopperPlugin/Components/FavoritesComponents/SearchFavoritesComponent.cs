using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class SearchFavoritesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "SearchFavorites";

        public SearchFavoritesComponent()
            : base(
                "SearchFavorites",
                "Search the Favorites of the project by a part of their name or of the name of a folder they " +
                "are in, optionally only those of one element type. Every input is optional; with none given, " +
                "every Favorite is listed.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Query",
                "Optional. Lists the Favorites whose name, or the name of a folder they are in, contains this " +
                "text (case-insensitive).");

            InText(
                "ElementType",
                "Optional. Lists only the Favorites of this element type (connect an ElementsType value list).");

            // No default value: an unconnected input must not limit.
            inManager.AddIntegerParameter(
                "Limit",
                "Limit",
                "Optional. The most Favorites to list (at least 1).".WithTypeName("integerItem"),
                GH_ParamAccess.item);

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
            OutTexts(
                "FavoriteNames",
                "Name of each matching Favorite.");

            OutTexts(
                "ElementTypes",
                "Element type of each matching Favorite.");

            OutTextTree(
                "Folders",
                "The folders leading from the root of the Favorites palette to each Favorite's folder (one " +
                "branch per Favorite; empty for a Favorite in the root folder).");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                1);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGet(
                    0,
                    out string query) &&
                !string.IsNullOrWhiteSpace(query))
            {
                parameters["query"] = query.Trim();
            }

            if (da.TryGet(
                    1,
                    out string elementType) &&
                !string.IsNullOrWhiteSpace(elementType))
            {
                parameters["elementType"] = elementType.Trim();
            }

            if (da.TryGet(
                    2,
                    out int limit))
            {
                if (limit < 1)
                {
                    this.AddError("Limit must be at least 1.");
                    return;
                }

                parameters["limit"] = limit;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var names = new List<object>();
            var elementTypes = new List<object>();
            var folderTree = new GH_Structure<GH_String>();

            var favorites = JsonOutputHelp.Items(
                response,
                "favorites");
            for (var i = 0; i < favorites.Count; i++)
            {
                var favorite = favorites[i];
                names.Add(JsonOutputHelp.Scalar(favorite, "name"));
                elementTypes.Add(JsonOutputHelp.Scalar(favorite, "elementType"));

                var path = new GH_Path(i);
                folderTree.EnsurePath(path);
                if (favorite["folder"] is JArray folder)
                {
                    foreach (var segment in folder)
                    {
                        folderTree.Append(
                            new GH_String(segment.ToString()),
                            path);
                    }
                }
            }

            da.SetDataList(0, names);
            da.SetDataList(1, elementTypes);
            da.SetDataTree(2, folderTree);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.FavoritesByType;

        public override Guid ComponentGuid =>
            new Guid("e3a71c52-8f4d-4b96-b0d2-5c19a8e6f7d4");
    }
}
