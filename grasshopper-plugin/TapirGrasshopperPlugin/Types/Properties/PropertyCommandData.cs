using Newtonsoft.Json;
using System.Collections.Generic;
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

    public class PropertyDefinitionUpdate
    {
        [JsonProperty("propertyId")]
        public PropertyGuidObject PropertyId;

        [JsonProperty("expressions")]
        public List<string> Expressions;
    }

    public class UpdatePropertyDefinitionsParameters
    {
        [JsonProperty("propertyDefinitions")]
        public List<PropertyDefinitionUpdate> PropertyDefinitions;
    }
}
