using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.ResponseTypes.Navigator;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public enum ElementFilter
    {
        NoFilter,
        IsEditable,
        IsVisibleByLayer,
        IsVisibleByRenovation,
        IsVisibleByStructureDisplay,
        IsVisibleIn3D,
        OnActualFloor,
        OnActualLayout,
        InMyWorkspace,
        IsIndependent,
        InCroppedView,
        HasAccessRight,
        IsOverriddenByRenovation
    }

    public class ElementFiltersObj : AcceptsElementFilters
    {
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