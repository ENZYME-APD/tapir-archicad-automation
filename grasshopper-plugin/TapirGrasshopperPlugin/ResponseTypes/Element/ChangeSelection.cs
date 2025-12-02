using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class ChangeSelectionParameters
    {
        [JsonProperty("addElementsToSelection")]
        public List<ElementGuidWrapper> AddElementsToSelection;

        [JsonProperty("removeElementsFromSelection")]
        public List<ElementGuidWrapper> RemoveElementsFromSelection;
    }
}