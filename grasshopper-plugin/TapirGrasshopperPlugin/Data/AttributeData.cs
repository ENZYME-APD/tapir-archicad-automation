using System.Collections.Generic;
using Newtonsoft.Json;
using TapirGrasshopperPlugin.ResponseTypes.IdObjects;

namespace TapirGrasshopperPlugin.Data
{
    public class AttributeGuidObject : GuidObject<AttributeGuidObject>
    {
    }

    public class AttributeGuidItemObject
        : GuidItemObject<AttributeGuidObject, AttributeGuidItemObject>
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
        : GuidItemsObject<AttributeGuidObject, AttributeGuidItemObject,
            AttributesObj>
    {
        [JsonProperty("attributes")]
        public List<AttributeGuidItemObject> Attributes;

        [JsonIgnore]
        public override List<AttributeGuidItemObject> GuidItems
        {
            get => Attributes;
            set => Attributes = value;
        }
    }

    public class AttributeGuidItemsObject
        : GuidItemsObject<AttributeGuidObject, AttributeGuidItemObject,
            AttributeGuidItemsObject>
    {
        [JsonProperty("attributeIds")]
        public List<AttributeGuidItemObject> AttributeIds;

        [JsonIgnore]
        public override List<AttributeGuidItemObject> GuidItems
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