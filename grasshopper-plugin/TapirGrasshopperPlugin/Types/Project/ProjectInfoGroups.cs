using Newtonsoft.Json;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Project
{
    // The identifier of a Project Info group (Archicad 30 and later).
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
}
