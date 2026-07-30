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
}
