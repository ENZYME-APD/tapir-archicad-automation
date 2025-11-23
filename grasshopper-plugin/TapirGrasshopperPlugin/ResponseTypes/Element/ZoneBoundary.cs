using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class ZoneBoundaryParameters
    {
        [JsonProperty("zoneElementId")]
        public ElementIdObj ZoneElementId;
    }

    public class ZoneBoundary
    {
        [JsonProperty("connectedElementId")]
        public ElementIdObj ConnectedElementId;

        [JsonProperty("isExternal")]
        public bool IsExternal;

        [JsonProperty("neighbouringZoneElementId")]
        public ElementIdObj NeighbouringZoneElementId;

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