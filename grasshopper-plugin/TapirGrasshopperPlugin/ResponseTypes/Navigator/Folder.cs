using Newtonsoft.Json;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.ResponseTypes.Navigator
{
    public class FolderParameters
    {
        [JsonProperty("name")]
        public string Name;
    }

    public class CreateViewMapFolderInput
    {
        [JsonProperty("folderParameters")]
        public FolderParameters FolderParameters;

        [JsonProperty(
            "parentNavigatorItemId",
            NullValueHandling = NullValueHandling.Ignore)]
        public NavigatorIdObj ParentNavigatorItemId;

        [JsonProperty(
            "previousNavigatorItemId",
            NullValueHandling = NullValueHandling.Ignore)]
        public NavigatorIdObj PreviousNavigatorItemId;
    }

    public class CreateViewMapFolderOutput
    {
        [JsonProperty("createdFolderNavigatorItemId")]
        public NavigatorIdObj CreatedFolderNavigatorItemId;
    }
}