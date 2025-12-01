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
        public NavigatorGuid ParentNavigatorItemId;

        [JsonProperty(
            "previousNavigatorItemId",
            NullValueHandling = NullValueHandling.Ignore)]
        public NavigatorGuid PreviousNavigatorItemId;
    }

    public class CreateViewMapFolderOutput
    {
        [JsonProperty("createdFolderNavigatorItemId")]
        public NavigatorGuid CreatedFolderNavigatorItemId;
    }
}