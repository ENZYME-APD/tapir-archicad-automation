using Newtonsoft.Json;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Types.Application
{
    public class CurrentWindowTypeResponse
    {
        [JsonProperty("currentWindowType")]
        public string CurrentWindowType;

        [JsonProperty("databaseId")]
        public DatabaseGuidObject DatabaseId;
    }
}
