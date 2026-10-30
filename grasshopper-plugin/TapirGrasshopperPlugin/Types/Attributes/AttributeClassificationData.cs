using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Types.Attributes
{
    public class AttributeClassificationObj
    {
        [JsonProperty("attributeId")]
        public AttributeGuidObject AttributeId;

        [JsonProperty("classificationId")]
        public ClassificationObj Classification;
    }

    public class SetClassificationsOfAttributesParameters
    {
        [JsonProperty("attributeClassifications")]
        public List<AttributeClassificationObj> AttributeClassifications;
    }

    public class GetClassificationsOfAttributesParameters
    {
        [JsonProperty("attributeIds")]
        public List<AttributeGuidWrapper> AttributeIds;
    }
}
