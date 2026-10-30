using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Navigator
{
    // Response of the GetRenovationFilters command: every renovation filter of
    // the project and the one the current window uses.
    public class RenovationFiltersResponse
    {
        [JsonProperty("renovationFilters")]
        public List<RenovationFilter> RenovationFilters { get; set; }

        // Absent when the current window has no renovation filter.
        [JsonProperty("activeRenovationFilterId")]
        public RenovationFilterGuid ActiveRenovationFilterId { get; set; }

        public RenovationFiltersResponse()
        {
            RenovationFilters = new List<RenovationFilter>();
        }
    }

    public class RenovationFilter
    {
        // The same identifier is the RenovationFilterGuid of the ViewSettings
        // and SetViewSettings components.
        [JsonProperty("renovationFilterId")]
        public RenovationFilterGuid RenovationFilterId { get; set; }

        [JsonProperty("name")]
        public string Name { get; set; }
    }

    // Input of the SetActiveRenovationFilter command: the filter by identifier
    // or by name. The null one is left out of the JSON.
    public class SetActiveRenovationFilterParameters
    {
        [JsonProperty("renovationFilterId", NullValueHandling = NullValueHandling.Ignore)]
        public RenovationFilterGuid RenovationFilterId { get; set; }

        [JsonProperty("name", NullValueHandling = NullValueHandling.Ignore)]
        public string Name { get; set; }
    }
}
