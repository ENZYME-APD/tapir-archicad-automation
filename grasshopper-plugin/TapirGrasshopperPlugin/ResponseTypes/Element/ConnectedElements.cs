using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class GetConnectedElementsParameters
    {
        [JsonProperty("elements")]
        public List<ElementIdItemObj> Elements;

        [JsonProperty("connectedElementType")]
        public string ConnectedElementType;
    }

    public class ConnectedElementsObj
    {
        [JsonProperty("connectedElements")]
        public List<ElementsObj> ConnectedElements;
    }
}