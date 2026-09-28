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

        [JsonProperty(
            "translatorName",
            NullValueHandling = NullValueHandling.Ignore)]
        public string TranslatorName;

        [JsonProperty(
            "elementsToExport",
            NullValueHandling = NullValueHandling.Ignore)]
        public string ElementsToExport;
    }

    public enum IFCElementsToExport
    {
        EntireProject,
        VisibleElementsOnAllStories,
        AllElementsOnCurrentStory,
        VisibleElementsOnCurrentStory,
        SelectedElementsOnly
    }

    public class IFCExportTranslator
    {
        [JsonProperty("name")]
        public string Name;

        [JsonProperty("preview")]
        public bool Preview;
    }

    public class GetIFCExportTranslatorsResponse
    {
        [JsonProperty("translators")]
        public List<IFCExportTranslator> Translators;
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
