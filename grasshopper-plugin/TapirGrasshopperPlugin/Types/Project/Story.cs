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

        [JsonProperty("dispOnSections")]
        public bool DispOnSections;
    }

    public class StoriesData
    {
        [JsonProperty("stories")]
        public List<StoryData> Stories;
    }
}