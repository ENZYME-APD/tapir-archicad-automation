using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.ResponseTypes.Project
{
    public class ProjectFilePathObject
    {
        [JsonProperty("projectFilePath")]
        public string ProjectFilePath { get; set; }
    }
}