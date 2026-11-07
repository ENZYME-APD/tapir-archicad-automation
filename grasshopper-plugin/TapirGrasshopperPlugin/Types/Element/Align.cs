using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    public static class AlignTo
    {
        public static readonly string[] Values =
        {
            "left",
            "right",
            "top",
            "bottom",
            "centerX",
            "centerY"
        };
    }

    public class AlignElementsParameters
    {
        [JsonProperty("elements")]
        public List<ElementGuidWrapper> Elements;

        [JsonProperty("alignTo")]
        public string AlignTo;

        [JsonProperty(
            "reference",
            NullValueHandling = NullValueHandling.Ignore)]
        public ElementGuid Reference;
    }
}
