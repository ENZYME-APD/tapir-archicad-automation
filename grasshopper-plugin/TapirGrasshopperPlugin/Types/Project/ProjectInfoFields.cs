using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Project
{
    // The identifier of a field of the Project Info groups (Archicad 30 and later).
    // It is not the autotext key that GetProjectInfoFields returns as projectInfoId.
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
