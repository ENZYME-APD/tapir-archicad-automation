using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class CollisionDetectionSettings
    {
        [JsonProperty("volumeTolerance")]
        public double VolumeTolerance;

        [JsonProperty("performSurfaceCheck")]
        public bool PerformSurfaceCheck;

        [JsonProperty("surfaceTolerance")]
        public double SurfaceTolerance;
    }

    public class GetCollisionsParameters
    {
        [JsonProperty("elementsGroup1")]
        public List<ElementIdItemObj> ElementsGroup1;

        [JsonProperty("elementsGroup2")]
        public List<ElementIdItemObj> ElementsGroup2;

        [JsonProperty("settings")]
        public CollisionDetectionSettings Settings;
    }

    public class Collision
    {
        [JsonProperty("elementId1")]
        public ElementIdObj ElementId1;

        [JsonProperty("elementId2")]
        public ElementIdObj ElementId2;

        [JsonProperty("hasBodyCollision")]
        public bool HasBodyCollision;

        [JsonProperty("hasClearenceCollision")]
        public bool HasClearenceCollision;
    }

    public class CollisionsOutput
    {
        [JsonProperty("collisions")]
        public List<Collision> Collisions;
    }
}