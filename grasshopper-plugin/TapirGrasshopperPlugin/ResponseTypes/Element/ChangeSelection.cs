using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class ChangeSelectionParameters
    {
        [JsonProperty("addElementsToSelection")]
        public List<ElementIdItemObj> AddElementsToSelection;

        [JsonProperty("removeElementsFromSelection")]
        public List<ElementIdItemObj> RemoveElementsFromSelection;
    }
}