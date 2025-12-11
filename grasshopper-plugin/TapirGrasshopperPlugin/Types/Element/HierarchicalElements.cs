using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class HierarchicalElementsObj
    {
        [JsonProperty("hierarchicalElements")]
        public List<ElementGuidWrapper> Elements;
    }
}