using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class FilterElementsObject : AcceptsElementFilters
    {
        [JsonProperty("elements")]
        public List<ElementGuidWrapper> Elements;

        [JsonProperty(
            "filters",
            NullValueHandling = NullValueHandling.Ignore)]
        private List<string> filters;

        [JsonIgnore]
        public List<string> Filters
        {
            get => filters;
            set => filters = AcceptElementFilters(value);
        }

        // The optional criteria below are sent only when given; an element is
        // kept when it matches all the given ones (any listed value matches).
        [JsonProperty(
            "layerIndices",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<int> LayerIndices;

        [JsonProperty(
            "floorIndices",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<int> FloorIndices;

        [JsonProperty(
            "elementTypes",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<string> ElementTypes;

        [JsonIgnore]
        public bool HasCriteria =>
            filters != null ||
            LayerIndices != null ||
            FloorIndices != null ||
            ElementTypes != null;
    }
}
