using Newtonsoft.Json;
using System.Collections.Generic;


namespace TapirGrasshopperPlugin.ResponseTypes.Navigator
{
    public class PublisherSetNamesObj
    {
        [JsonProperty("publisherSetNames")]
        public List<string> PublisherSetNames;
    }
}