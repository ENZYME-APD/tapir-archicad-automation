using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.Types.Navigator
{
    public class ViewMapFolderOutput
    {
        [JsonProperty("createdFolderNavigatorItemId")]
        public NavigatorGuid CreatedFolderNavigatorItemId;
    }
}