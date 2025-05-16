using System;
using System.Collections.Generic;
using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.Data
{
    public class AttributeIdObj : IdObj<AttributeIdObj> { }

    public class AttributeDetail
    {
        [JsonProperty ("attributeId")]
        public AttributeIdObj AttributeId;

        [JsonProperty ("index")]
        public uint Index;

        [JsonProperty ("name")]
        public string Name;
    }

    public class AttributesObj
    {
        [JsonProperty ("attributes")]
        public List<AttributeDetail> Attributes;
    }
}
