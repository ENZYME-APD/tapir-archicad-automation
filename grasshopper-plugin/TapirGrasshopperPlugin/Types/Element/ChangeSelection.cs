using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class ChangeSelectionParameters
    {
        [JsonProperty("addElementsToSelection")]
        public List<ElementGuidWrapper> AddElementsToSelection;

        [JsonProperty("removeElementsFromSelection")]
        public List<ElementGuidWrapper> RemoveElementsFromSelection;
    }
}