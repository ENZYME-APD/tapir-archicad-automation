using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Project
{
    public class StoryData
    {
        [JsonProperty("name")]
        public string Name;

        [JsonProperty("level")]
        public double Level;

        // Null when the add-on predates the field.
        [JsonProperty("height")]
        public double? Height;

        [JsonProperty("dispOnSections")]
        public bool DispOnSections;
    }

    public class StoriesData
    {
        [JsonProperty("stories")]
        public List<StoryData> Stories;
    }
}