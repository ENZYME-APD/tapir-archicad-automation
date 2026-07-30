using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Types.Properties
{
    public class AttributePropertyValueObj
    {
        [JsonProperty("attributeId")]
        public AttributeGuidObject AttributeId;

        [JsonProperty("propertyId")]
        public PropertyGuidObject PropertyId;

        [JsonProperty("propertyValue")]
        public PropertyValueObj PropertyValue;
    }

    public class SetPropertyValuesOfAttributesParameters
    {
        [JsonProperty("attributePropertyValues")]
        public List<AttributePropertyValueObj> AttributePropertyValues;
    }

    public class GetPropertyValuesOfAttributesParameters
    {
        [JsonProperty("attributeIds")]
        public List<AttributeGuidWrapper> AttributeIds;

        [JsonProperty("properties")]
        public List<PropertyGuidWrapper> Properties;
    }
}
