using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Attributes
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

    public class AttributesObject
        : GuidItemsObject<AttributeGuidObject, AttributeGuidWrapper,
            AttributesObject>
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

        // Seconds since 1970-01-01 00:00:00 UTC.
        [JsonProperty("modificationTime")]
        public long ModificationTime;

        // Only from Archicad 30, and only for an attribute Archicad keeps a
        // creation record of; null otherwise.
        [JsonProperty("creationInfo")]
        public AttributeCreationInfo CreationInfo;
    }

    public class AttributeCreationInfo
    {
        [JsonProperty("method")]
        public string Method;

        [JsonProperty("methodDescription")]
        public string MethodDescription;

        [JsonProperty("source")]
        public string Source;

        [JsonProperty("creatorUserName")]
        public string CreatorUserName;

        // Seconds since 1970-01-01 00:00:00 UTC.
        [JsonProperty("creationTime")]
        public long CreationTime;
    }

    public class AttributeDetailsObject
    {
        [JsonProperty("attributes")]
        public List<AttributeDetail> Attributes;
    }
}