using Newtonsoft.Json;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.MEP
{
    // The system groups and physical systems of the MEP API are neither
    // elements nor attributes, so their identifiers have their own types
    // (the components accept them, the wrapped form and guid texts alike).
    public class MEPSystemGroupGuid : GuidObject<MEPSystemGroupGuid>
    {
    }

    public class MEPSystemGroupGuidWrapper
        : GuidWrapper<MEPSystemGroupGuid, MEPSystemGroupGuidWrapper>
    {
        [JsonProperty("systemGroupId")]
        public MEPSystemGroupGuid SystemGroupId;

        [JsonIgnore]
        public override MEPSystemGroupGuid Id
        {
            get => SystemGroupId;
            set => SystemGroupId = value;
        }
    }

    public class MEPPhysicalSystemGuid : GuidObject<MEPPhysicalSystemGuid>
    {
    }

    public class MEPPhysicalSystemGuidWrapper
        : GuidWrapper<MEPPhysicalSystemGuid, MEPPhysicalSystemGuidWrapper>
    {
        [JsonProperty("physicalSystemId")]
        public MEPPhysicalSystemGuid PhysicalSystemId;

        [JsonIgnore]
        public override MEPPhysicalSystemGuid Id
        {
            get => PhysicalSystemId;
            set => PhysicalSystemId = value;
        }
    }
}
