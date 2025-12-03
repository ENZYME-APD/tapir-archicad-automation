using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Helps
{
    public static class GDLParameterHelps
    {
        public static ElementsWithGDLParametersInput
            ToElementsWithGDLParameters(
                this ElementsObject elementsObject,
                List<string> parameterNames,
                List<object> values)
        {
            var result = new ElementsWithGDLParametersInput
            {
                ElementsWithGDLParameters =
                    new List<ElementWithGDLParameters>()
            };

            for (int i = 0; i < parameterNames.Count; i++)
            {
                var item = new ElementWithGDLParameters
                {
                    ElementId = elementsObject.GuidItems[i].ElementId,
                    GdlParameterList = new GdlParameterArray()
                    {
                        new GdlParameterDetails
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
}