using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Navigator
{
    // An add-on viewpoint item of the Project Map: a root, a group or a leaf
    // node created through the Archicad API by an add-on.
    public class AddOnViewpointGuid : GuidObject<AddOnViewpointGuid>
    {
    }

    public class AddOnViewpointGuidWrapper
        : GuidWrapper<AddOnViewpointGuid, AddOnViewpointGuidWrapper>
    {
        [JsonProperty("viewpointId")]
        public AddOnViewpointGuid ViewpointId;

        [JsonIgnore]
        public override AddOnViewpointGuid Id
        {
            get => ViewpointId;
            set => ViewpointId = value;
        }
    }

    public class AddOnViewpointsObject
        : GuidItemsObject<AddOnViewpointGuid, AddOnViewpointGuidWrapper,
            AddOnViewpointsObject>
    {
        [JsonProperty("viewpointIds")]
        public List<AddOnViewpointGuidWrapper> ViewpointIds;

        [JsonIgnore]
        public override List<AddOnViewpointGuidWrapper> GuidWrappers
        {
            get => ViewpointIds;
            set => ViewpointIds = value;
        }
    }
}
