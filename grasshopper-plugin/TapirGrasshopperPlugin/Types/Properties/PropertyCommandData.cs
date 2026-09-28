using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Properties
{
    public class PropertyGroupGuidObject : GuidObject<PropertyGroupGuidObject>
    {
    }

    public class PropertyGroupGuidWrapper
        : GuidWrapper<PropertyGroupGuidObject, PropertyGroupGuidWrapper>
    {
        [JsonProperty("propertyGroupId")]
        public PropertyGroupGuidObject PropertyGroupId;

        [JsonIgnore]
        public override PropertyGroupGuidObject Id
        {
            get => PropertyGroupId;
            set => PropertyGroupId = value;
        }
    }

    public class DeletePropertyGroupsParameters
    {
        [JsonProperty("propertyGroupIds")]
        public List<PropertyGroupGuidWrapper> PropertyGroupIds;
    }

    public class DeletePropertyDefinitionsParameters
    {
        [JsonProperty("propertyIds")]
        public List<PropertyGuidWrapper> PropertyIds;
    }

    public class PropertyGroupToCreate
    {
        [JsonProperty("name")]
        public string Name;

        [JsonProperty(
            "description",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Description;
    }

    public class PropertyGroupArrayItem
    {
        [JsonProperty("propertyGroup")]
        public PropertyGroupToCreate PropertyGroup;
    }

    public class CreatePropertyGroupsParameters
    {
        [JsonProperty("propertyGroups")]
        public List<PropertyGroupArrayItem> PropertyGroups;
    }

    public class EnumValueGuidWrapper
    {
        [JsonProperty("enumValueId")]
        public EnumValueGuidObject EnumValueId;
    }

    public class EnumValueToAdd
    {
        [JsonProperty("displayValue")]
        public string DisplayValue;
    }

    public class EnumValueToAddArrayItem
    {
        [JsonProperty("enumValue")]
        public EnumValueToAdd EnumValue;
    }

    public class EnumValueRename
    {
        [JsonProperty("enumValueId")]
        public EnumValueGuidObject EnumValueId;

        [JsonProperty("displayValue")]
        public string DisplayValue;
    }

    public class PropertyAvailabilityUpdate
    {
        [JsonProperty(
            "set",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<ClassificationItemGuidWrapper> Set;

        [JsonProperty(
            "add",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<ClassificationItemGuidWrapper> Add;

        [JsonProperty(
            "remove",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<ClassificationItemGuidWrapper> Remove;
    }

    // Only the fields given change; the others are left out of the JSON.
    public class PropertyDefinitionUpdate
    {
        [JsonProperty("propertyId")]
        public PropertyGuidObject PropertyId;

        [JsonProperty(
            "name",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Name;

        [JsonProperty(
            "description",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Description;

        [JsonProperty(
            "groupId",
            NullValueHandling = NullValueHandling.Ignore)]
        public PropertyGroupGuidObject GroupId;

        [JsonProperty(
            "defaultValue",
            NullValueHandling = NullValueHandling.Ignore)]
        public JObject DefaultValue;

        [JsonProperty(
            "expressions",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<string> Expressions;

        [JsonProperty(
            "availability",
            NullValueHandling = NullValueHandling.Ignore)]
        public PropertyAvailabilityUpdate Availability;

        [JsonProperty(
            "possibleEnumValues",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<EnumValueToAddArrayItem> PossibleEnumValues;

        [JsonProperty(
            "renameEnumValues",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<EnumValueRename> RenameEnumValues;

        [JsonProperty(
            "removeEnumValues",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<EnumValueGuidWrapper> RemoveEnumValues;

        [JsonProperty(
            "enumOrder",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<string> EnumOrder;
    }

    public class UpdatePropertyDefinitionsParameters
    {
        [JsonProperty("propertyDefinitions")]
        public List<PropertyDefinitionUpdate> PropertyDefinitions;
    }

    public class PropertyGroupUpdate
    {
        [JsonProperty("propertyGroupId")]
        public PropertyGroupGuidObject PropertyGroupId;

        [JsonProperty(
            "name",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Name;

        [JsonProperty(
            "description",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Description;
    }

    public class UpdatePropertyGroupsParameters
    {
        [JsonProperty("propertyGroups")]
        public List<PropertyGroupUpdate> PropertyGroups;
    }

    public class ImportPropertiesXmlParameters
    {
        [JsonProperty("xml")]
        public string Xml;

        [JsonProperty("conflictPolicy")]
        public string ConflictPolicy;
    }
}
