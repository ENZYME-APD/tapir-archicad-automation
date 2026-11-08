using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Types.Element
{
    // The input of GetElementsByCriteria. Every criterion is optional and sent
    // only when given; an element is listed when it matches all of them.
    public class ElementsByCriteriaObject : AcceptsElementFilters
    {
        [JsonProperty(
            "types",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<string> Types;

        [JsonProperty(
            "floorIndices",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<int> FloorIndices;

        [JsonProperty(
            "layerIndices",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<int> LayerIndices;

        [JsonProperty(
            "filters",
            NullValueHandling = NullValueHandling.Ignore)]
        private List<string> filters;

        [JsonIgnore]
        public List<string> Filters
        {
            get => filters;
            set => filters = value == null ? null : AcceptElementFilters(value);
        }

        [JsonProperty(
            "propertyCriteria",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<PropertyCriterionObject> PropertyCriteria;

        [JsonProperty(
            "countOnly",
            NullValueHandling = NullValueHandling.Ignore)]
        public bool? CountOnly;

        [JsonProperty(
            "limit",
            NullValueHandling = NullValueHandling.Ignore)]
        public int? Limit;
    }

    // One property criterion; the property is given by its id, or by its name
    // (and group name, when the name is used in more than one group).
    public class PropertyCriterionObject
    {
        [JsonProperty(
            "propertyId",
            NullValueHandling = NullValueHandling.Ignore)]
        public PropertyGuidObject PropertyId;

        [JsonProperty(
            "propertyName",
            NullValueHandling = NullValueHandling.Ignore)]
        public string PropertyName;

        [JsonProperty(
            "propertyGroupName",
            NullValueHandling = NullValueHandling.Ignore)]
        public string PropertyGroupName;

        [JsonProperty(
            "operator",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Operator;

        [JsonProperty(
            "value",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Value;
    }

    public class ElementsByCriteriaResponse
    {
        [JsonProperty("count")]
        public int Count;

        [JsonProperty(
            "elements",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<ElementGuidWrapper> Elements;
    }
}
