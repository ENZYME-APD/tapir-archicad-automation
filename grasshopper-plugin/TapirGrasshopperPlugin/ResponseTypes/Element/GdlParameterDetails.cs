using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class GdlParameterDetails
    {
        [JsonProperty("name")]
        public string Name;

        [JsonProperty("index")]
        public string Index;

        [JsonProperty("type")]
        public string Type;

        [JsonProperty("dimension1")]
        public int Dimension1;

        [JsonProperty("dimension2")]
        public int Dimension2;

        [JsonProperty("value")]
        public object Value;
    }

    public class ElementsWithGDLParametersInput
    {
        [JsonProperty("elementsWithGDLParameters")]
        public List<ElementWithGDLParameters> ElementsWithGDLParameters
        {
            get;
            set;
        }
    }

    public class ElementWithGDLParameters
    {
        [JsonProperty("elementId")]
        public ElementGuid ElementId { get; set; }

        [JsonProperty("gdlParameters")]
        public GdlParameterArray GdlParameterList { get; set; }
    }

    public class GdlParameterArray : List<GdlParameterDetails>
    {
    }

    public class GDLHolder
    {
        public string ElementId { get; set; }
        public GdlParameterDetails GdlParameterDetails { get; set; }

        public GDLHolder(
            string id,
            GdlParameterDetails details)
        {
            ElementId = id;
            GdlParameterDetails = details;
        }
    }

    public class GDLParametersResponse
    {
        [JsonProperty("gdlParametersOfElements")]
        public List<GdlParameterList> GdlLists;
    }

    public class GdlParameterList
    {
        [JsonProperty("parameters")]
        public GdlParameterArray GdlParameterArray;
    }
}