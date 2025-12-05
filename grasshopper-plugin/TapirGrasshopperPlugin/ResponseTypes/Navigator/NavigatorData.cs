using System.Collections.Generic;
using Newtonsoft.Json;
using TapirGrasshopperPlugin.ResponseTypes.GuidObjects;

namespace TapirGrasshopperPlugin.ResponseTypes.Navigator
{
    public class NavigatorGuid : GuidObject<NavigatorGuid>
    {
    }

    public class NavigatorGuidWrapper
        : GuidWrapper<NavigatorGuid, NavigatorGuidWrapper>
    {
        [JsonProperty("navigatorItemId")]
        public NavigatorGuid NavigatorId;

        [JsonIgnore]
        public override NavigatorGuid Id
        {
            get => NavigatorId;
            set => NavigatorId = value;
        }
    }

    public class NavigatorItemsObject
        : GuidItemsObject<NavigatorGuid, NavigatorGuidWrapper,
            NavigatorItemsObject>
    {
        [JsonProperty("navigatorItemIds")]
        public List<NavigatorGuidWrapper> NavigatorGuidWrappers;

        [JsonIgnore]
        public override List<NavigatorGuidWrapper> GuidWrappers
        {
            get => NavigatorGuidWrappers;
            set => NavigatorGuidWrappers = value;
        }
    }
}