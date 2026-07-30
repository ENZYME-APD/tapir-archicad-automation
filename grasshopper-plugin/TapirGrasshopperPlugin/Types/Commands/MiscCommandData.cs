using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Types.Commands
{
    // Library

    public class LibraryFileAddition
    {
        [JsonProperty("inputPath")]
        public string InputPath;

        [JsonProperty("outputPath")]
        public string OutputPath;
    }

    public class AddFilesToEmbeddedLibraryParameters
    {
        [JsonProperty("files")]
        public List<LibraryFileAddition> Files;
    }

    // Grouping

    public class ElementGroupParameters
    {
        [JsonProperty("elements")]
        public List<ElementGuidWrapper> Elements;
    }

    public class CreateGroupsParameters
    {
        [JsonProperty("elementGroups")]
        public List<ElementGroupParameters> ElementGroups;
    }

    // Favorites

    public class ImportFavoritesParameters
    {
        [JsonProperty("path")]
        public string Path;

        [JsonProperty(
            "targetFolder",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<string> TargetFolder;

        [JsonProperty(
            "importFolders",
            NullValueHandling = NullValueHandling.Ignore)]
        public bool? ImportFolders;

        [JsonProperty(
            "conflictPolicy",
            NullValueHandling = NullValueHandling.Ignore)]
        public string ConflictPolicy;
    }

    public class ExportFavoritesParameters
    {
        [JsonProperty("path")]
        public string Path;

        [JsonProperty(
            "names",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<string> Names;
    }

    // Solid element operations

    public class SolidLink
    {
        [JsonProperty("targetId")]
        public ElementGuid TargetId;

        [JsonProperty("operatorId")]
        public ElementGuid OperatorId;

        [JsonProperty(
            "operation",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Operation;
    }

    public class SolidLinksParameters
    {
        [JsonProperty("solidLinks")]
        public List<SolidLink> SolidLinks;
    }
}
