using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class GdlParameterDetails
    {
        [JsonProperty("name")]
        public string Name;

        [JsonProperty("index")]
        public int Index;

        [JsonProperty("type")]
        public string Type;

        [JsonProperty("dimension1")]
        public int Dimension1;

        [JsonProperty("dimension2")]
        public int Dimension2;

        [JsonProperty("value")]
        public object Value;
    }

    public class SetGdlParameterDetails
    {
        [JsonProperty("name",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Name;

        [JsonProperty("index",
            NullValueHandling = NullValueHandling.Ignore)]
        public int Index;
    }

    public class SetGdlParameterDetailsInteger : SetGdlParameterDetails
    {
        [JsonProperty("value")]
        public int Value;
    }

    public class SetGdlParameterDetailsDouble : SetGdlParameterDetails
    {
        [JsonProperty("value")]
        public double Value;
    }

    public class SetGdlParameterDetailsString : SetGdlParameterDetails
    {
        [JsonProperty("value")]
        public string Value;
    }

    public class SetGdlParameterDetailsBoolean : SetGdlParameterDetails
    {
        [JsonProperty("value")]
        public bool Value;
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
        public SetGdlParameterArray GdlParameterList { get; set; }
    }

    public class SetGdlParameterArray : List<SetGdlParameterDetails>
    {
    }

    public class GdlParameterArray : List<GdlParameterDetails>
    {
    }

    public class GDLHolder
    {
        public ElementGuid ElementId { get; set; }
        public GdlParameterDetails GdlParameterDetails { get; set; }

        public GDLHolder(
            ElementGuid id,
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