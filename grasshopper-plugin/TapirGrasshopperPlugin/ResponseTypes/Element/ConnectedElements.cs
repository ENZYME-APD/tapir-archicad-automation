using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class GetConnectedElementsParameters
    {
        [JsonProperty("elements")]
        public List<ElementGuidItemObject> Elements;

        [JsonProperty("connectedElementType")]
        public string ConnectedElementType;
    }

    public class ConnectedElementsObj
    {
        [JsonProperty("connectedElements")]
        public List<ElementsObj> ConnectedElements;
    }
}