using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class FilterElementsObj : AcceptsElementFilters
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
    }
}