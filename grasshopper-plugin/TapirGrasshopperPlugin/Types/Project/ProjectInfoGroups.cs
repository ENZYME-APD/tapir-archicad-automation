using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Project
{
    // The Project Info groups and their fields of Archicad 30 (GetProjectInfoGroups
    // and the group and field commands). A field's guid here is not the autotext key
    // that GetProjectInfoFields returns as projectInfoId.

    public class ProjectInfoGroupGuid : GuidObject<ProjectInfoGroupGuid>
    {
    }

    public class ProjectInfoGroupGuidWrapper
        : GuidWrapper<ProjectInfoGroupGuid, ProjectInfoGroupGuidWrapper>
    {
        [JsonProperty("projectInfoGroupId")]
        public ProjectInfoGroupGuid ProjectInfoGroupId;

        [JsonIgnore]
        public override ProjectInfoGroupGuid Id
        {
            get => ProjectInfoGroupId;
            set => ProjectInfoGroupId = value;
        }
    }

    public class ProjectInfoGroupsObject
        : GuidItemsObject<ProjectInfoGroupGuid, ProjectInfoGroupGuidWrapper, ProjectInfoGroupsObject>
    {
        [JsonProperty("projectInfoGroups")]
        public List<ProjectInfoGroupGuidWrapper> ProjectInfoGroups;

        [JsonIgnore]
        public override List<ProjectInfoGroupGuidWrapper> GuidWrappers
        {
            get => ProjectInfoGroups;
            set => ProjectInfoGroups = value;
        }
    }

    public class ProjectInfoFieldGuid : GuidObject<ProjectInfoFieldGuid>
    {
    }

    public class ProjectInfoFieldGuidWrapper
        : GuidWrapper<ProjectInfoFieldGuid, ProjectInfoFieldGuidWrapper>
    {
        [JsonProperty("projectInfoFieldId")]
        public ProjectInfoFieldGuid ProjectInfoFieldId;

        [JsonIgnore]
        public override ProjectInfoFieldGuid Id
        {
            get => ProjectInfoFieldId;
            set => ProjectInfoFieldId = value;
        }
    }

    public class ProjectInfoGroupField
    {
        [JsonProperty("projectInfoFieldId")]
        public ProjectInfoFieldGuid ProjectInfoFieldId;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("value")]
        public string Value;

        // The autotext key of the field (the id SetProjectInfoField takes); given
        // only when exactly one project info autotext has the field's name.
        [JsonProperty("projectInfoId")]
        public string ProjectInfoId;
    }

    public class ProjectInfoGroupDetails
    {
        [JsonProperty("projectInfoGroupId")]
        public ProjectInfoGroupGuid ProjectInfoGroupId;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("fields")]
        public List<ProjectInfoGroupField> Fields;
    }

    public class GetProjectInfoGroupsResponse
    {
        [JsonProperty("projectInfoGroups")]
        public List<ProjectInfoGroupDetails> ProjectInfoGroups;
    }

    // Input parameter payloads

    public class ProjectInfoGroupToCreate
    {
        [JsonProperty("name")]
        public string Name;
    }

    public class CreateProjectInfoGroupsParameters
    {
        [JsonProperty("projectInfoGroups")]
        public List<ProjectInfoGroupToCreate> ProjectInfoGroups;
    }

    public class RenameProjectInfoGroupParameter
    {
        [JsonProperty("projectInfoGroupId")]
        public ProjectInfoGroupGuid ProjectInfoGroupId;

        [JsonProperty("name")]
        public string Name;
    }

    public class RenameProjectInfoGroupsParameters
    {
        [JsonProperty("projectInfoGroups")]
        public List<RenameProjectInfoGroupParameter> ProjectInfoGroups;
    }

    public class MoveProjectInfoGroupParameter
    {
        [JsonProperty("projectInfoGroupId")]
        public ProjectInfoGroupGuid ProjectInfoGroupId;

        // Omitted when the group goes to the end of the group list.
        [JsonProperty(
            "beforeProjectInfoGroupId",
            NullValueHandling = NullValueHandling.Ignore)]
        public ProjectInfoGroupGuid BeforeProjectInfoGroupId;
    }

    public class MoveProjectInfoGroupsParameters
    {
        [JsonProperty("projectInfoGroups")]
        public List<MoveProjectInfoGroupParameter> ProjectInfoGroups;
    }

    public class RenameProjectInfoFieldParameter
    {
        [JsonProperty("projectInfoFieldId")]
        public ProjectInfoFieldGuid ProjectInfoFieldId;

        [JsonProperty("name")]
        public string Name;
    }

    public class RenameProjectInfoFieldsParameters
    {
        [JsonProperty("projectInfoFields")]
        public List<RenameProjectInfoFieldParameter> ProjectInfoFields;
    }

    public class MoveProjectInfoFieldParameter
    {
        [JsonProperty("projectInfoFieldId")]
        public ProjectInfoFieldGuid ProjectInfoFieldId;

        [JsonProperty("projectInfoGroupId")]
        public ProjectInfoGroupGuid ProjectInfoGroupId;

        // Omitted when the field goes to the end of the group.
        [JsonProperty(
            "beforeProjectInfoFieldId",
            NullValueHandling = NullValueHandling.Ignore)]
        public ProjectInfoFieldGuid BeforeProjectInfoFieldId;
    }

    public class MoveProjectInfoFieldsParameters
    {
        [JsonProperty("projectInfoFields")]
        public List<MoveProjectInfoFieldParameter> ProjectInfoFields;
    }
}
