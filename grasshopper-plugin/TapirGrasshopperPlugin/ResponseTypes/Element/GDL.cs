using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class GDLParametersResponse
    {
        [JsonProperty("gdlParametersOfElements")]
        public List<GDLParameterList> GdlParametersOfElements;
    }

    public class GDLParameterList
    {
        [JsonProperty("parameters")]
        public GDLParameters Parameters;

        public override string ToString()
        {
            return string.Join(
                ", ",
                Parameters.Select(x => x.Name));
        }
    }

    public class GDLParameters : List<GDLParameterDetails>
    {
    }

    public class GDLParameterDetails
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
        public Object Value;
    }


    public class ElementsWithGDLParameters : List<ElementWithGDLParameters>
    {
    }

    public class ElementWithGDLParameters
    {
        [JsonProperty("elementId")]
        public ElementIdItemObj ElementId { get; set; }

        [JsonProperty("gdlParameters")]
        public GDLParameters GDLParameters { get; set; }
    }
}