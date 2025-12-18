using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class ZoneBoundary
    {
        [JsonProperty("connectedElementId")]
        public ElementGuid ConnectedElementId;

        [JsonProperty("isExternal")]
        public bool IsExternal;

        [JsonProperty("neighbouringZoneElementId")]
        public ElementGuid NeighbouringZoneElementId;

        [JsonProperty("area")]
        public double Area;

        [JsonProperty("polygonOutline")]
        public List<Point3D> PolygonCoordinates;
    }

    public class ZoneBoundariesOutput
    {
        [JsonProperty("zoneBoundaries")]
        public List<ZoneBoundary> ZoneBoundaries;
    }
}