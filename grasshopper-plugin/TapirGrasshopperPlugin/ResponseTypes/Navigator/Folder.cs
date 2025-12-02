using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.ResponseTypes.Navigator
{
    public class ViewMapFolderOutput
    {
        [JsonProperty("createdFolderNavigatorItemId")]
        public NavigatorGuid CreatedFolderNavigatorItemId;
    }
}