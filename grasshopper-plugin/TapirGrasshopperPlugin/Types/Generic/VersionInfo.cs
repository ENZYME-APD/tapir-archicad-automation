using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.Types.Generic
{
    public class VersionInfo
    {
        [JsonProperty("version")]
        public string Version { get; set; }
    }
}