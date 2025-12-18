using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class GetConnectedElementsParameters
    {
        [JsonProperty("elements")]
        public List<ElementGuidWrapper> Elements;

        [JsonProperty("connectedElementType")]
        public string ConnectedElementType;
    }

    public class ConnectedElementsObject
    {
        [JsonProperty("connectedElements")]
        public List<ElementsObject> ConnectedElements;
    }
}