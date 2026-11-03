using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class ElementAtCoordinateInput : AcceptsElementFilters
    {
        [JsonProperty("elementType")]
        public string ElementType;

        [JsonProperty("coordinate")]
        public Point2D Coordinate;

        [JsonProperty("z")]
        public double Z;

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

    public class ElementAtCoordinateOutput
    {
        [JsonProperty("found")]
        public bool Found;

        // Absent (null) when nothing was found.
        [JsonProperty("elementId")]
        public ElementGuid ElementId;
    }
}
