using Newtonsoft.Json;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class GroupGuid : GuidObject<GroupGuid>
    {
    }

    public class GroupGuidWrapper
        : GuidWrapper<GroupGuid, GroupGuidWrapper>
    {
        [JsonProperty("groupId")]
        public GroupGuid GroupId;

        [JsonIgnore]
        public override GroupGuid Id
        {
            get => GroupId;
            set => GroupId = value;
        }
    }
}
