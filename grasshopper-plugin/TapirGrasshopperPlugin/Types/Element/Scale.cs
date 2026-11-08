using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class ScaleElementsParameters
    {
        [JsonProperty("elements")]
        public List<ElementGuidWrapper> Elements;

        [JsonProperty("center")]
        public Point2D Center;

        [JsonProperty("factor")]
        public double Factor;

        // Only sent when given; it must equal Factor.
        [JsonProperty(
            "factorY",
            NullValueHandling = NullValueHandling.Ignore)]
        public double? FactorY;
    }
}
