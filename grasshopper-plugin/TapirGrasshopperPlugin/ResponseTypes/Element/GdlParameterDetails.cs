using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public static class GDLParameterHelps
    {
        public static List<GDLHolder> ToGdlHolders(
            this GDLParametersResponse response,
            List<string> ids,
            string parameterName)
        {
            var result = new List<GDLHolder>();

            for (int i = 0; i < ids.Count(); i++)
            {
                var id = ids[0];
                var pList = response.GdlLists[0];

                if (pList.GdlParameterArray == null ||
                    pList.GdlParameterArray.Count == 0)
                {
                    continue;
                }

                result.AddRange(
                    pList
                        .GdlParameterArray.Where(x => x.Name == parameterName)
                        .Select(details => new GDLHolder(
                            id,
                            details)));
            }

            return result;
        }
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
        public List<GdlParameterDetails> GdlParameterArray;
    }

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
        public GdlParameterList GdlDetailList { get; set; }
    }
}