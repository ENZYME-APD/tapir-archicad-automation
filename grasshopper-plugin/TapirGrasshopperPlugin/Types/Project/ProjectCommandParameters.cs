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

        // The Project Info group to create the field in (Archicad 30 or newer);
        // omitted, the field goes to the first group.
        [JsonProperty(
            "projectInfoGroupId",
            NullValueHandling = NullValueHandling.Ignore)]
        public ProjectInfoGroupGuid ProjectInfoGroupId;
    }

    public class CreateProjectInfoFieldsParameters
    {
        [JsonProperty("projectInfoFields")]
        public List<ProjectInfoFieldToCreate> ProjectInfoFields;
    }

    public class DeleteProjectInfoFieldsParameters
    {
        // Autotext keys, as GetProjectInfoFields returns them.
        [JsonProperty(
            "projectInfoIds",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<string> ProjectInfoIds;

        // Fields of the Project Info groups (Archicad 30 or newer), as
        // GetProjectInfoGroups returns them.
        [JsonProperty(
            "projectInfoFieldIds",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<ProjectInfoFieldGuidWrapper> ProjectInfoFieldIds;
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
