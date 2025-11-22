using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.ResponseTypes.Attributes
{
    public class ContainedLayerObj
    {
        [JsonProperty("attributeId")]
        public AttributeIdObj AttributeId;

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
        public AttributeIdObj AttributeId;

        [JsonProperty("attributeIndex")]
        public int AttributeIndex;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("layers")]
        public List<ContainedLayerObj> Layers;
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

    public class LayerCombinationDataObj
    {
        [JsonProperty("attributeId")]
        public AttributeIdObj AttributeId;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("layers")]
        public List<ContainedLayerObj> Layers;
    }

    public class LayerCombinationDataArrayObj
    {
        [JsonProperty("layerCombinationDataArray")]
        public List<LayerCombinationDataObj> LayerCombinationDataArray;

        [JsonProperty("overwriteExisting")]
        public bool OverwriteExisting;
    }
}