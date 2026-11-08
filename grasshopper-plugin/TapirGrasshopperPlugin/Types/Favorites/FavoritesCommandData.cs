using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Types.Favorites
{
    public class FavoriteForElement
    {
        [JsonProperty("elementId")]
        public ElementGuid ElementId;

        [JsonProperty("favorite")]
        public string Favorite;
    }

    public class ApplyFavoritesToElementsParameters
    {
        // Either favoritesToApply, or favorite with elements and/or filter.
        [JsonProperty(
            "favoritesToApply",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<FavoriteForElement> FavoritesToApply;

        [JsonProperty(
            "favorite",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Favorite;

        [JsonProperty(
            "elements",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<ElementGuidWrapper> Elements;

        [JsonProperty(
            "filter",
            NullValueHandling = NullValueHandling.Ignore)]
        public ElementTypeFloorLayerFilter Filter;

        [JsonProperty(
            "applySettings",
            NullValueHandling = NullValueHandling.Ignore)]
        public bool? ApplySettings;

        [JsonProperty(
            "applyClassifications",
            NullValueHandling = NullValueHandling.Ignore)]
        public bool? ApplyClassifications;

        [JsonProperty(
            "applyCategories",
            NullValueHandling = NullValueHandling.Ignore)]
        public bool? ApplyCategories;

        [JsonProperty(
            "applyProperties",
            NullValueHandling = NullValueHandling.Ignore)]
        public bool? ApplyProperties;

        [JsonProperty(
            "keepFields",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<string> KeepFields;
    }

    public class UpdateFavoritesFromElementsParameters
    {
        [JsonProperty("favoritesFromElements")]
        public List<FavoriteForElement> FavoritesFromElements;
    }

    public class FavoriteRename
    {
        [JsonProperty("oldName")]
        public string OldName;

        [JsonProperty("newName")]
        public string NewName;
    }

    public class RenameFavoritesParameters
    {
        [JsonProperty("renames")]
        public List<FavoriteRename> Renames;
    }

    public class DeleteFavoritesParameters
    {
        [JsonProperty("favorites")]
        public List<string> Favorites;
    }
}
