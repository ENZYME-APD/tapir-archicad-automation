using Newtonsoft.Json;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.ResponseTypes.Navigator
{
    public class RenameNavigatorItemInput
    {
        [JsonProperty("navigatorItemId")]
        public NavigatorIdObj NavigatorItemId;

        [JsonProperty(
            "newName",
            NullValueHandling = NullValueHandling.Ignore)]
        public string NewName;

        [JsonProperty(
            "newId",
            NullValueHandling = NullValueHandling.Ignore)]
        public string NewId;
    }
}