using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.ResponseTypes.Generic
{
    public class LocationInfo
    {
        [JsonProperty("archicadLocation")]
        public string ArchicadLocation { get; set; }
    }
}