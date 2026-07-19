using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.Types.Application
{
    public class CurrentWindowTypeResponse
    {
        [JsonProperty("currentWindowType")]
        public string CurrentWindowType;
    }
}
