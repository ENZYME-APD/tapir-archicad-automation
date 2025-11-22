using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

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