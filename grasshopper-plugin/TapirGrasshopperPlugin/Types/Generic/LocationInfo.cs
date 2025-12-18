using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.Types.Generic
{
    public class LocationInfo
    {
        [JsonProperty("archicadLocation")]
        public string ArchicadLocation { get; set; }
    }
}