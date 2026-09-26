using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class ClassificationSystemGuidWrapper
        : GuidWrapper<ClassificationGuid, ClassificationSystemGuidWrapper>
    {
        [JsonProperty("classificationSystemId")]
        public ClassificationGuid ClassificationSystemId;

        [JsonIgnore]
        public override ClassificationGuid Id
        {
            get => ClassificationSystemId;
            set => ClassificationSystemId = value;
        }
    }

    public class ClassificationItemGuidWrapper
        : GuidWrapper<ClassificationGuid, ClassificationItemGuidWrapper>
    {
        [JsonProperty("classificationItemId")]
        public ClassificationGuid ClassificationItemId;

        [JsonIgnore]
        public override ClassificationGuid Id
        {
            get => ClassificationItemId;
            set => ClassificationItemId = value;
        }
    }

    public class DeleteClassificationSystemsParameters
    {
        [JsonProperty("classificationSystemIds")]
        public List<ClassificationSystemGuidWrapper> ClassificationSystemIds;
    }

    public class DeleteClassificationItemsParameters
    {
        [JsonProperty("classificationItemIds")]
        public List<ClassificationItemGuidWrapper> ClassificationItemIds;
    }

    // Only the fields given change; the others are left out of the JSON.
    public class ClassificationSystemUpdate
    {
        [JsonProperty("classificationSystemId")]
        public ClassificationGuid ClassificationSystemId;

        [JsonProperty(
            "name",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Name;

        [JsonProperty(
            "description",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Description;

        [JsonProperty(
            "source",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Source;

        [JsonProperty(
            "version",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Version;

        [JsonProperty(
            "date",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Date;
    }

    public class UpdateClassificationSystemsParameters
    {
        [JsonProperty("classificationSystems")]
        public List<ClassificationSystemUpdate> ClassificationSystems;
    }

    // Only the fields given change; the others are left out of the JSON.
    public class ClassificationItemUpdate
    {
        [JsonProperty("classificationItemId")]
        public ClassificationGuid ClassificationItemId;

        [JsonProperty(
            "id",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Id;

        [JsonProperty(
            "name",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Name;

        [JsonProperty(
            "description",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Description;
    }

    public class UpdateClassificationItemsParameters
    {
        [JsonProperty("classificationItems")]
        public List<ClassificationItemUpdate> ClassificationItems;
    }

    public class ImportClassificationsXmlParameters
    {
        [JsonProperty("xml")]
        public string Xml;

        [JsonProperty("systemConflictPolicy")]
        public string SystemConflictPolicy;

        [JsonProperty("itemConflictPolicy")]
        public string ItemConflictPolicy;
    }
}
