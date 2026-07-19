using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Types.Commands
{
    public class IFCFileOperationParameters
    {
        [JsonProperty("method")]
        public string Method;

        [JsonProperty("ifcFilePath")]
        public string IfcFilePath;

        [JsonProperty(
            "fileType",
            NullValueHandling = NullValueHandling.Ignore)]
        public string FileType;
    }

    public class GetElementsByIFCIdsParameters
    {
        [JsonProperty("ifcIds")]
        public List<string> IfcIds;
    }

    public class GetCurrentRevisionChangesOfLayoutsParameters
    {
        [JsonProperty("layoutDatabaseIds")]
        public List<DatabaseGuidWrapper> LayoutDatabaseIds;
    }
}
