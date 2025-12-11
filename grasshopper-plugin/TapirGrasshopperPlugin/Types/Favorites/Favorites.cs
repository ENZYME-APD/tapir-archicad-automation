using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Types.Favorites
{
    public class FavoritesObject
    {
        [JsonProperty("favorites")]
        public Favorites Favorites { get; set; }

        public FavoritesObject()
        {
            Favorites = new Favorites();
        }

        public FavoritesObject(
            IEnumerable<string> strings)
        {
            Favorites = new Favorites(strings);
        }

        public static FavoritesObject FromWrappers(
            List<GH_ObjectWrapper> wrappers)
        {
            return new FavoritesObject(wrappers.Select(x => x.AsString()));
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

    public class FavoritesFromElementsObject
    {
        [JsonProperty("favoritesFromElements")]
        public List<FavoriteFromElementObj> FavoritesFromElements { get; set; }

        public FavoritesFromElementsObject()
        {
            FavoritesFromElements = new List<FavoriteFromElementObj>();
        }

        public FavoritesFromElementsObject(
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