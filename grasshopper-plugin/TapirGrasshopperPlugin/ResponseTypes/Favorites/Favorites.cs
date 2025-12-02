using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.ResponseTypes.Favorites
{
    public class FavoritesObj
    {
        [JsonProperty("favorites")]
        public Favorites Favorites { get; set; }

        public FavoritesObj()
        {
            Favorites = new Favorites();
        }

        public FavoritesObj(
            IEnumerable<string> strings)
        {
            Favorites = new Favorites(strings);
        }

        public static FavoritesObj FromWrappers(
            List<GH_ObjectWrapper> wrappers)
        {
            return new FavoritesObj(wrappers.Select(x => x.AsString()));
        }
    }

    public class Favorites : List<string>
    {
        public Favorites()
        {
        }

        public Favorites(
            IEnumerable<string> strings)
        {
            AddRange(strings);
        }
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
            List<ElementGuidWrapper> ids,
            List<string> favorites)
        {
            FavoritesFromElements = ids
                .Zip(
                    favorites,
                    (
                        id,
                        fav) => new FavoriteFromElementObj
                    {
                        ElementGuid = id, Favorite = fav
                    })
                .ToList();
        }
    }

    public class FavoriteFromElementObj
    {
        [JsonProperty("elementId")]
        public ElementGuidWrapper ElementGuid;

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