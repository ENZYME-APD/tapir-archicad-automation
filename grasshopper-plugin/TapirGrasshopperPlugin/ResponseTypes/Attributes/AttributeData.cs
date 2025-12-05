using System.Collections.Generic;
using Newtonsoft.Json;
using TapirGrasshopperPlugin.ResponseTypes.GuidObjects;

namespace TapirGrasshopperPlugin.ResponseTypes.Attributes
{
    public class AttributeGuidObject : GuidObject<AttributeGuidObject>
    {
    }

    public class AttributeGuidWrapper
        : GuidWrapper<AttributeGuidObject, AttributeGuidWrapper>
    {
        [JsonProperty("attributeId")]
        public AttributeGuidObject AttributeId;

        [JsonIgnore]
        public override AttributeGuidObject Id
        {
            get => AttributeId;
            set => AttributeId = value;
        }
    }

    public class AttributesObj
        : GuidItemsObject<AttributeGuidObject, AttributeGuidWrapper,
            AttributesObj>
    {
        [JsonProperty("attributes")]
        public List<AttributeGuidWrapper> Attributes;

        [JsonIgnore]
        public override List<AttributeGuidWrapper> GuidWrappers
        {
            get => Attributes;
            set => Attributes = value;
        }
    }

    public class AttributeGuidItemsObject
        : GuidItemsObject<AttributeGuidObject, AttributeGuidWrapper,
            AttributeGuidItemsObject>
    {
        [JsonProperty("attributeIds")]
        public List<AttributeGuidWrapper> AttributeIds;

        [JsonIgnore]
        public override List<AttributeGuidWrapper> GuidWrappers
        {
            get => AttributeIds;
            set => AttributeIds = value;
        }
    }

    public class AttributeDetail
    {
        [JsonProperty("attributeId")]
        public AttributeGuidObject AttributeId;

        [JsonProperty("index")]
        public uint Index;

        [JsonProperty("name")]
        public string Name;
    }

    public class AttributeDetailsObj
    {
        [JsonProperty("attributes")]
        public List<AttributeDetail> Attributes;
    }
}