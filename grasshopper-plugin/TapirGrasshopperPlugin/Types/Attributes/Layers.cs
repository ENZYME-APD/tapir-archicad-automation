using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Attributes
{
    public class LayerDataObject
    {
        [JsonProperty("attributeId")]
        public AttributeGuidObject AttributeId;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("isHidden")]
        public bool IsHidden;

        [JsonProperty("isLocked")]
        public bool IsLocked;

        [JsonProperty("isWireframe")]
        public bool IsWireframe;

        [JsonProperty("intersectionGroupNr")]
        public int IntersectionGroupNr;
    }

    public class LayerDataArrayObject
    {
        [JsonProperty("layerDataArray")]
        public List<LayerDataObject> LayerDataArray;

        [JsonProperty("overwriteExisting")]
        public bool OverwriteExisting;
    }

    public class LayerDetailsObj
    {
        [JsonProperty("name")]
        public string Name;

        [JsonProperty("isHidden")]
        public bool IsHidden;

        [JsonProperty("isLocked")]
        public bool IsLocked;

        [JsonProperty("isWireframe")]
        public bool IsWireframe;

        [JsonProperty("intersectionGroupNr")]
        public int IntersectionGroupNr;
    }

    public class LayerObj
    {
        [JsonProperty("layerAttribute")]
        public LayerDetailsObj LayerAttribute;
    }

    public class LayersObject
    {
        [JsonProperty("attributes")]
        public List<LayerObj> Attributes;
    }
}