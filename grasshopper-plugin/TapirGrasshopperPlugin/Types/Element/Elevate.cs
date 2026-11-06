using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class ElementWithElevationParameters
    {
        [JsonProperty("elementId")]
        public ElementGuid Element;

        [JsonProperty("elevation")]
        public double Elevation;

        [JsonProperty("copy")]
        public bool Copy;
    }

    public class ElevateElementsParameters
    {
        [JsonProperty("elementsWithElevations")]
        public List<ElementWithElevationParameters> ElementsWithElevations;
    }
}
