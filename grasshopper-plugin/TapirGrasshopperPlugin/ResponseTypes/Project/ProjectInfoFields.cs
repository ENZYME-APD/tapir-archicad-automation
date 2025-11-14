using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Project
{
    public class ProjectInfoField
    {
        [JsonProperty ("projectInfoId")]
        public string ProjectInfoId { get; set; }

        [JsonProperty ("projectInfoName")]
        public string ProjectInfoName { get; set; }

        [JsonProperty ("projectInfoValue")]
        public string ProjectInfoValue { get; set; }
    }

    public class ProjectInfoFields
    {
        [JsonProperty ("fields")]
        public List<ProjectInfoField> Fields { get; set; }
    }
}
