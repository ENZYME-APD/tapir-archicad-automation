using Newtonsoft.Json;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.ResponseTypes.Navigator
{
    public class MoveNavigatorItemInput
    {
        [JsonProperty("navigatorItemIdToMove")]
        public NavigatorIdObj NavigatorItemIdToMove;

        [JsonProperty("parentNavigatorItemId")]
        public NavigatorIdObj ParentNavigatorItemId;

        [JsonProperty(
            "previousNavigatorItemId",
            NullValueHandling = NullValueHandling.Ignore)]
        public NavigatorIdObj PreviousNavigatorItemId;
    }
}