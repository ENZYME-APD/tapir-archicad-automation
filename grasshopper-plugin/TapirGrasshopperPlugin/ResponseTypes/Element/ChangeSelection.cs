using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class ChangeSelectionParameters
    {
        [JsonProperty("addElementsToSelection")]
        public List<ElementGuidItemObject> AddElementsToSelection;

        [JsonProperty("removeElementsFromSelection")]
        public List<ElementGuidItemObject> RemoveElementsFromSelection;
    }
}