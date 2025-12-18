using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Attributes
{
    public class ContainedLayerObject
    {
        [JsonProperty("attributeId")]
        public AttributeGuidObject AttributeId;

        [JsonProperty("isHidden")]
        public bool IsHidden;

        [JsonProperty("isLocked")]
        public bool IsLocked;

        [JsonProperty("isWireframe")]
        public bool IsWireframe;

        [JsonProperty("intersectionGroupNr")]
        public int IntersectionGroupNr;
    }

    public class LayerCombinationDetailsObj
    {
        [JsonProperty("attributeId")]
        public AttributeGuidObject AttributeId;

        [JsonProperty("attributeIndex")]
        public int AttributeIndex;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("layers")]
        public List<ContainedLayerObject> Layers;
    }

    public class LayerCombinationObj
    {
        [JsonProperty("layerCombination")]
        public LayerCombinationDetailsObj LayerCombination;
    }

    public class LayerCombinationsObj
    {
        [JsonProperty("layerCombinations")]
        public List<LayerCombinationObj> LayerCombinations;
    }

    public class LayerCombinationDataObject
    {
        [JsonProperty("attributeId")]
        public AttributeGuidObject AttributeId;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("layers")]
        public List<ContainedLayerObject> Layers;
    }

    public class LayerCombinationDataArrayObject
    {
        [JsonProperty("layerCombinationDataArray")]
        public List<LayerCombinationDataObject> LayerCombinationDataArray;

        [JsonProperty("overwriteExisting")]
        public bool OverwriteExisting;
    }
}