using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Favorites
{
    public class FavoritesResponse
    {
        [JsonProperty("favorites")]
        public Favorites Favorites { get; set; }
    }

    public class Favorites : List<string>
    {
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