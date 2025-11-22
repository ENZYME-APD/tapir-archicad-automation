using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.ResponseTypes.Generic
{
    public class VersionInfo
    {
        [JsonProperty("version")]
        public string Version { get; set; }
    }
}