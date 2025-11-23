using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class NewElementId
    {
        [JsonProperty("guid")]
        public string Guid { get; set; }
    }

    public class NewElementWithGDLParameters
    {
        [JsonProperty("elementId")]
        public NewElementId ElementId { get; set; }

        [JsonProperty("gdlParameters")]
        public List<GdlParameterDetails> GdlParameters
        {
            get;
            set;
        }
    }

    public class NewElementsWithGDLParameters
    {
        [JsonProperty("elementsWithGDLParameters")]
        public List<NewElementWithGDLParameters> ElementsWithGDLParameters
        {
            get;
            set;
        }
    }
}