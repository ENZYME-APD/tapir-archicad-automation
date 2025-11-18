using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.ResponseTypes.Project
{
    public class ProjectInfo
    {
        [JsonProperty("isUntitled")]
        public bool IsUntitled { get; set; }

        [JsonProperty("isTeamwork")]
        public bool IsTeamwork { get; set; }

        [JsonProperty("projectLocation")]
        public string ProjectLocation { get; set; }

        [JsonProperty("projectPath")]
        public string ProjectPath { get; set; }

        [JsonProperty("projectName")]
        public string ProjectName { get; set; }
    }
}