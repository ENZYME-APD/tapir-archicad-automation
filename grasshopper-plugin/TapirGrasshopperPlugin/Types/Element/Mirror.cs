using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class MirrorAxis
    {
        [JsonProperty("beginPoint")]
        public Point2D BeginPoint;

        [JsonProperty("endPoint")]
        public Point2D EndPoint;
    }

    public class ElementWithMirrorAxisParameters
    {
        [JsonProperty("elementId")]
        public ElementGuid Element;

        [JsonProperty("mirrorAxis")]
        public MirrorAxis MirrorAxis;

        [JsonProperty("copy")]
        public bool Copy;
    }

    public class MirrorElementsParameters
    {
        [JsonProperty("elementsWithMirrorAxes")]
        public List<ElementWithMirrorAxisParameters> ElementsWithMirrorAxes;
    }
}
