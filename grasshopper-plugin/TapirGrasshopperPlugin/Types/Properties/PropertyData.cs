using Newtonsoft.Json;
using System.Collections.Generic;
using System.ComponentModel;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Properties
{
    public class PropertyGuidObject : GuidObject<PropertyGuidObject>
    {
    }

    public class PropertyGuidWrapper
        : GuidWrapper<PropertyGuidObject, PropertyGuidWrapper>
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

    public class PropertiesObject
        : GuidItemsObject<PropertyGuidObject, PropertyGuidWrapper,
            PropertiesObject>
    {
        [JsonProperty("properties")]
        public List<PropertyGuidWrapper> Properties;

        [JsonIgnore]
        public override List<PropertyGuidWrapper> GuidWrappers
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

        [JsonProperty("propertyGroupId")]
        public PropertyGroupGuidObject PropertyGroupId;

        [JsonProperty("propertyDescription")]
        public string PropertyDescription;

        // Only for custom, non expression-based properties whose default is set.
        [JsonProperty("defaultValueDisplay")]
        public string DefaultValueDisplay;

        // Only for custom properties.
        [JsonProperty("availability")]
        public List<ClassificationItemGuidWrapper> Availability;

        // Only for custom enumeration properties whose default is set.
        [JsonProperty("defaultEnumValueIds")]
        public List<EnumValueGuidObject> DefaultEnumValueIds;

        [JsonProperty("possibleEnumValues")]
        public List<PossibleEnumValueItem> PossibleEnumValues;
    }

    public class EnumValueGuidObject : GuidObject<EnumValueGuidObject>
    {
    }

    public class PossibleEnumValue
    {
        [JsonProperty("guid")]
        public string Guid;

        [JsonProperty("displayValue")]
        public string DisplayValue;

        [JsonProperty("nonLocalizedValue")]
        public string NonLocalizedValue;
    }

    public class PossibleEnumValueItem
    {
        [JsonProperty("enumValue")]
        public PossibleEnumValue EnumValue;
    }

    public class PropertyGroupDetails
    {
        [JsonProperty("propertyGroupId")]
        public PropertyGroupGuidObject PropertyGroupId;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("description")]
        public string Description;

        [JsonProperty("isCustom")]
        public bool IsCustom;
    }

    public class AllProperties
    {
        [JsonProperty("properties")]
        public List<PropertyDetailsObj> Properties { get; set; }

        [JsonProperty("propertyGroups")]
        public List<PropertyGroupDetails> PropertyGroups { get; set; }
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