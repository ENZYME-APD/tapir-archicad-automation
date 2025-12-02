using Newtonsoft.Json;
using System.Collections.Generic;

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
        public List<ElementGuidWrapper> ElementsGroup1;

        [JsonProperty("elementsGroup2")]
        public List<ElementGuidWrapper> ElementsGroup2;

        [JsonProperty("settings")]
        public CollisionDetectionSettings Settings;
    }

    public class Collision
    {
        [JsonProperty("elementId1")]
        public ElementGuid ElementId1;

        [JsonProperty("elementId2")]
        public ElementGuid ElementId2;

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