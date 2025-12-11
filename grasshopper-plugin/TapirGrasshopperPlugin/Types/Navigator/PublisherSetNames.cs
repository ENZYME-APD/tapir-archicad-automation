using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Navigator
{
    public class PublisherSetNamesObject
    {
        [JsonProperty("publisherSetNames")]
        public List<string> PublisherSetNames;
    }
}