using System.Collections.Generic;
using System.ComponentModel;
using Newtonsoft.Json;
using TapirGrasshopperPlugin.ResponseTypes.IdObjects;

namespace TapirGrasshopperPlugin.Data
{
    public class PropertyGuidObject : GuidObject<PropertyGuidObject>
    {
    }

    public class PropertyGuidItemObject
        : GuidItemObject<PropertyGuidObject, PropertyGuidItemObject>
    {
        [JsonProperty("propertyId")]
        public PropertyGuidObject PropertyId;

        [JsonIgnore]
        public override PropertyGuidObject Id
        {
            get => PropertyId;
            set => PropertyId = value;
        }
    }

    public class PropertiesObj
        : GuidItemsObject<PropertyGuidObject, PropertyGuidItemObject,
            PropertiesObj>
    {
        [JsonProperty("properties")]
        public List<PropertyGuidItemObject> Properties;

        [JsonIgnore]
        public override List<PropertyGuidItemObject> GuidItems
        {
            get => Properties;
            set => Properties = value;
        }
    }

    public class PropertyValueObj
    {
        [JsonProperty("value")]
        public string Value;
    }

    public class PropertyDetailsObj
    {
        public override string ToString()
        {
            return PropertyId + "; " + PropertyGroupName + "; " + PropertyName;
        }

        [JsonProperty("propertyId")]
        public PropertyGuidObject PropertyId;

        [JsonProperty("propertyGroupName")]
        public string PropertyGroupName;

        [JsonProperty("propertyName")]
        public string PropertyName;
    }

    public class AllProperties
    {
        [JsonProperty("properties")]
        public List<PropertyDetailsObj> Properties { get; set; }
    }

    public class PropertyValue
    {
        [JsonProperty("value")]
        public string Value;
    }

    public class PropertyValueOrError
    {
        [JsonProperty(
            "propertyValue",
            DefaultValueHandling = DefaultValueHandling.Populate)]
        [DefaultValue(null)]
        public PropertyValue PropertyValue;
    }

    public class PropertyValues
    {
        [JsonProperty(
            "propertyValues",
            DefaultValueHandling = DefaultValueHandling.Populate)]
        [DefaultValue(null)]
        public List<PropertyValueOrError> PropertyValuesOrErrors;
    }

    public class PropertyValuesForElements
    {
        [JsonProperty("propertyValuesForElements")]
        public List<PropertyValues> PropertyValuesOrErrors { get; set; }
    }
}