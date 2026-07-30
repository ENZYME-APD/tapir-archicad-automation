using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Project
{
    public class ProjectInfoFieldToCreate
    {
        [JsonProperty("projectInfoName")]
        public string ProjectInfoName;

        [JsonProperty(
            "projectInfoValue",
            NullValueHandling = NullValueHandling.Ignore)]
        public string ProjectInfoValue;
    }

    public class CreateProjectInfoFieldsParameters
    {
        [JsonProperty("projectInfoFields")]
        public List<ProjectInfoFieldToCreate> ProjectInfoFields;
    }

    public class DeleteProjectInfoFieldsParameters
    {
        [JsonProperty("projectInfoIds")]
        public List<string> ProjectInfoIds;
    }

    public class RebuildViewParameters
    {
        [JsonProperty("regenerate")]
        public bool Regenerate;
    }

    public class PrintViewParameters
    {
        [JsonProperty("grid")]
        public bool Grid;

        [JsonProperty("fixText")]
        public bool FixText;

        [JsonProperty("scale")]
        public int Scale;

        [JsonProperty("printArea")]
        public string PrintArea;
    }
}
