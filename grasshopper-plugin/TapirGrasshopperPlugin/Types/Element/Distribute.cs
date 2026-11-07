using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    public static class DistributeDirection
    {
        public static readonly string[] Values =
        {
            "horizontal",
            "vertical"
        };
    }

    public static class DistributeMode
    {
        public static readonly string[] Values =
        {
            "centers",
            "gaps"
        };
    }

    public class DistributeElementsParameters
    {
        [JsonProperty("elements")]
        public List<ElementGuidWrapper> Elements;

        [JsonProperty("direction")]
        public string Direction;

        [JsonProperty("mode")]
        public string Mode;
    }
}
