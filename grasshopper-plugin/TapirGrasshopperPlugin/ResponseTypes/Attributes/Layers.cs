using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.ResponseTypes.Attributes
{
    public class LayerDataObj
    {
        [JsonProperty("attributeId")]
        public AttributeIdObj AttributeId;

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

    public class LayerDataArrayObj
    {
        [JsonProperty("layerDataArray")]
        public List<LayerDataObj> LayerDataArray;

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

    public class LayersObj
    {
        [JsonProperty("attributes")]
        public List<LayerObj> Attributes;
    }
}