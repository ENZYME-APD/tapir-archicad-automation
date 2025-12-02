using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class HierarchicalElementsObj
    {
        [JsonProperty("hierarchicalElements")]
        public List<ElementGuidWrapper> Elements;
    }
}