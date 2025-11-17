using System.Collections.Generic;
using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.Data
{
    public class AttributeIdObj : IdObj<AttributeIdObj> { }

    public class AttributeIdItemObj : IdItemObj<AttributeIdObj, AttributeIdItemObj>
    {
        [JsonProperty ("attributeId")]
        public AttributeIdObj AttributeId;

        [JsonIgnore]
        public override AttributeIdObj Id
        {
            get { return AttributeId; }
            set { AttributeId = value; }
        }
    }

    public class AttributesObj : IdsObj<AttributeIdObj, AttributeIdItemObj, AttributesObj>
    {
        [JsonProperty ("attributes")]
        public List<AttributeIdItemObj> Attributes;

        [JsonIgnore]
        public override List<AttributeIdItemObj> Ids
        {
            get { return Attributes; }
            set { Attributes = value; }
        }
    }

    public class AttributeIdsObj : IdsObj<AttributeIdObj, AttributeIdItemObj, AttributeIdsObj>
    {
        [JsonProperty ("attributeIds")]
        public List<AttributeIdItemObj> AttributeIds;

        [JsonIgnore]
        public override List<AttributeIdItemObj> Ids
        {
            get { return AttributeIds; }
            set { AttributeIds = value; }
        }
    }

    public class AttributeDetail
    {
        [JsonProperty ("attributeId")]
        public AttributeIdObj AttributeId;

        [JsonProperty ("index")]
        public uint Index;

        [JsonProperty ("name")]
        public string Name;
    }

    public class AttributeDetailsObj
    {
        [JsonProperty ("attributes")]
        public List<AttributeDetail> Attributes;
    }
}
