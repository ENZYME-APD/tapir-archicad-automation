using Newtonsoft.Json;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.ResponseTypes.Favorites
{
    public class FavoritesObj
    {
        [JsonProperty("favorites")]
        public Favorites Favorites { get; set; }
    }

    public class Favorites : List<string>
    {
    }

    public class FavoritesFromElementsObj
    {
        [JsonProperty("favoritesFromElements")]
        public List<FavoriteFromElementObj> FavoritesFromElements { get; set; }

        public FavoritesFromElementsObj()
        {
            FavoritesFromElements = new List<FavoriteFromElementObj>();
        }

        public FavoritesFromElementsObj(
            List<ElementIdItemObj> ids,
            List<string> favorites)
        {
            FavoritesFromElements = ids
                .Zip(
                    favorites,
                    (
                        id,
                        fav) => new FavoriteFromElementObj
                    {
                        ElementId = id, Favorite = fav
                    })
                .ToList();
        }
    }

    public class FavoriteFromElementObj
    {
        [JsonProperty("elementId")]
        public ElementIdItemObj ElementId;

        [JsonProperty("favorite")]
        public string Favorite;
    }

    public class ElementTypeObject
    {
        [JsonProperty("elementType")]
        public string ElementType;

        public ElementTypeObject() { }

        public ElementTypeObject(
            string elementType)
        {
            ElementType = elementType;
        }
    }
}