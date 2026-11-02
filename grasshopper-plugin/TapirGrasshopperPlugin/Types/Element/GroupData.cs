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

    // An element set: a hidden, persistent collection of elements that
    // add-ons create to link elements together.
    public class ElementSetGuid : GuidObject<ElementSetGuid>
    {
    }

    public class ElementSetGuidWrapper
        : GuidWrapper<ElementSetGuid, ElementSetGuidWrapper>
    {
        [JsonProperty("elementSetId")]
        public ElementSetGuid ElementSetId;

        [JsonIgnore]
        public override ElementSetGuid Id
        {
            get => ElementSetId;
            set => ElementSetId = value;
        }
    }
}
