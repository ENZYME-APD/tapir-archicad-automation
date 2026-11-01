using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    // The response of GetHotspotsOfElements: the hotspots of each element
    // (the points the cursor snaps to and dimensions attach to), or an
    // error per element.

    public class Hotspot
    {
        // World coordinates.
        [JsonProperty("coordinate")]
        public Point3D Coordinate;

        // Which kind of point of the element the hotspot is (API_NeigID).
        [JsonProperty("neigId")]
        public int NeigId;

        // Subindex of the point within its kind (API_Neig inIndex).
        [JsonProperty("inIndex")]
        public int InIndex;
    }

    public class HotspotsOfElementItem
    {
        [JsonProperty(
            "hotspots",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<Hotspot> Hotspots;

        [JsonProperty(
            "error",
            NullValueHandling = NullValueHandling.Ignore)]
        public Error Error;
    }

    public class HotspotsOfElementsObject
    {
        [JsonProperty("hotspotsOfElements")]
        public List<HotspotsOfElementItem> HotspotsOfElements;
    }
}
