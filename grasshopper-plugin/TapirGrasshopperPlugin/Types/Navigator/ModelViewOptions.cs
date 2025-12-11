using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Navigator
{
    public class ModelViewOptionsResponse
    {
        [JsonProperty("modelViewOptions")]
        public List<ModelViewOption> ModelViewOptions { get; set; }
    }

    public class ModelViewOption
    {
        [JsonProperty("name")]
        public string Name { get; set; }
    }
}