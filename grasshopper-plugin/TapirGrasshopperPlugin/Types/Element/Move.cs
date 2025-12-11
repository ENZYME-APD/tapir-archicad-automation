using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class Vector3D
    {
        [JsonProperty("x")]
        public double X;

        [JsonProperty("y")]
        public double Y;

        [JsonProperty("z")]
        public double Z;
    }

    public class ElementWithMoveParameters
    {
        [JsonProperty("elementId")]
        public ElementGuid Element;

        [JsonProperty("moveVector")]
        public Vector3D MoveVector;

        [JsonProperty("copy")]
        public bool Copy;
    }

    public class MoveElementsParameters
    {
        [JsonProperty("elementsWithMoveVectors")]
        public List<ElementWithMoveParameters> ElementsWithMoveParameters;
    }
}