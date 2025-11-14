using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Navigator
{
    public class ModelViewOptionsResponse
    {
        public static string Doc => "Gets all model view options.";

        [JsonProperty ("modelViewOptions")]
        public List<ModelViewOption> ModelViewOptions { get; set; }
    }

    public class ModelViewOption
    {
        [JsonProperty ("name")]
        public string Name { get; set; }
    }
}
