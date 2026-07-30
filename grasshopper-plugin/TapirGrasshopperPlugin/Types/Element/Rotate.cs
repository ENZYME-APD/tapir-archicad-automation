using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class Rotation
    {
        [JsonProperty("beginPoint")]
        public Point2D BeginPoint;

        [JsonProperty("endPoint")]
        public Point2D EndPoint;

        [JsonProperty("origin")]
        public Point2D Origin;
    }

    public class ElementWithRotationParameters
    {
        [JsonProperty("elementId")]
        public ElementGuid Element;

        [JsonProperty("rotation")]
        public Rotation Rotation;

        [JsonProperty("copy")]
        public bool Copy;
    }

    public class RotateElementsParameters
    {
        [JsonProperty("elementsWithRotations")]
        public List<ElementWithRotationParameters> ElementsWithRotations;
    }
}
