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

        // Optional in SetStories, always present in the GetStories response.
        [JsonProperty(
            "index",
            NullValueHandling = NullValueHandling.Ignore)]
        public int? Index;

        // Only in the GetStories response, from Archicad 28.
        [JsonProperty(
            "ifcId",
            NullValueHandling = NullValueHandling.Ignore)]
        public string IfcId;
    }

    public class StoriesData
    {
        [JsonProperty("stories")]
        public List<StoryData> Stories;
    }
}