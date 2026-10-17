using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    // The response of GetZoneReductions: the area reductions of each Zone
    // (the polygons the Zone Stamp's area calculation subtracts, with their
    // percentage and area), or an error per Zone.

    public class ZoneReductionHole
    {
        [JsonProperty("polygonOutline")]
        public List<Point2D> PolygonOutline;

        [JsonProperty(
            "polygonArcs",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<Arc> PolygonArcs;
    }

    public class ZoneReduction
    {
        // Wall, Column, Hatch, Gable, LowArea, Rest or Unknown.
        [JsonProperty("type")]
        public string Type;

        [JsonProperty("percent")]
        public double Percent;

        [JsonProperty("area")]
        public double Area;

        [JsonProperty("polygonOutline")]
        public List<Point2D> PolygonOutline;

        [JsonProperty(
            "polygonArcs",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<Arc> PolygonArcs;

        [JsonProperty("holes")]
        public List<ZoneReductionHole> Holes;
    }

    public class ReductionsOfZoneItem
    {
        [JsonProperty(
            "reductions",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<ZoneReduction> Reductions;

        [JsonProperty(
            "error",
            NullValueHandling = NullValueHandling.Ignore)]
        public Error Error;
    }

    public class ReductionsOfZonesObject
    {
        [JsonProperty("reductionsOfZones")]
        public List<ReductionsOfZoneItem> ReductionsOfZones;
    }
}
