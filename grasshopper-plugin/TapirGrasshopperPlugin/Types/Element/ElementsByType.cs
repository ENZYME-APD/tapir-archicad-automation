using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class ElementsByTypeObj : AcceptsElementFilters
    {
        [JsonProperty("elementType")]
        public string ElementType;

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

        [JsonProperty(
            "databases",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<DatabaseGuidWrapper> Databases;
    }
}