using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.ResponseTypes.Attributes
{
    public class AttributesByTypeObj
    {
        [JsonProperty("attributeType")]
        public string AttributeType;
    }
}