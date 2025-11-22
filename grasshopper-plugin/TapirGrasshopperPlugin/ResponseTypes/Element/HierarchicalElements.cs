using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class HierarchicalElementsObj
    {
        [JsonProperty("hierarchicalElements")]
        public List<ElementIdItemObj> Elements;
    }
}