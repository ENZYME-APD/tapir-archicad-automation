using Newtonsoft.Json;
using System.Collections.Generic;
using System.Linq;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public static class GDLParameterHelps
    {
        public static NewElementsWithGDLParameters ToElementsWithGDLParameters(
            this ElementsObj elementsObj,
            List<string> parameterNames,
            List<object> values)
        {
            var result = new NewElementsWithGDLParameters
            {
                ElementsWithGDLParameters =
                    new List<NewElementWithGDLParameters>()
            };

            for (int i = 0; i < parameterNames.Count; i++)
            {
                var item = new NewElementWithGDLParameters
                {
                    ElementId =
                        new NewElementId
                        {
                            Guid = elementsObj.GuidItems[i].ElementId
                                .Guid
                        },
                    GdlParameters = new List<GdlParameterDetails>()
                    {
                        new GdlParameterDetails()
                        {
                            Name = parameterNames[i],
                            Value = values[i]
                        }
                    }
                };

                result.ElementsWithGDLParameters.Add(item);
            }

            return result;
        }

        public static List<GDLHolder> ToGdlHolders(
            this GDLParametersResponse response,
            List<string> ids,
            string parameterName)
        {
            var result = new List<GDLHolder>();

            if (response?.GdlLists == null || ids == null)
            {
                return result;
            }

            for (var i = 0; i < ids.Count; i++)
            {
                var id = ids[i];
                var pList = response.GdlLists[i];

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
        public object Value;
    }

    public class ElementsWithGDLParametersInput
    {
        [JsonProperty("elementsWithGDLParameters")]
        public ElementsWithGDLParameters ElementsWithGDLParameters { get; set; }
    }

    public class ElementsWithGDLParameters : List<ElementWithGDLParameters>
    {
    }

    public class ElementWithGDLParameters
    {
        [JsonProperty("elementId")]
        public ElementGuidItemObject ElementGuid { get; set; }

        [JsonProperty("gdlParameters")]
        public GdlParameterList GdlDetailList { get; set; }
    }
}