using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Helps
{
    public static class GDLParameterHelps
    {
        public static ElementsWithGDLParametersInput ToSetInput(
            this List<GDLHolder> gdlHolders,
            object value)
        {
            var result = new ElementsWithGDLParametersInput
            {
                ElementsWithGDLParameters =
                    new List<ElementWithGDLParameters>()
            };

            foreach (var holder in gdlHolders)
            {
                var changedParameters = holder.GdlParameterDetails;
                changedParameters.Value = value;

                var item = new ElementWithGDLParameters
                {
                    ElementId = holder.ElementId,
                    GdlParameterList = new GdlParameterArray
                    {
                        changedParameters
                    }
                };

                result.ElementsWithGDLParameters.Add(item);
            }

            return result;
        }

        public static List<GDLHolder> ToGdlHolders(
            this GDLParametersResponse response,
            List<ElementGuid> ids,
            string parameterName)
        {
            var result = new List<GDLHolder>();

            if (response?.GdlLists == null || ids == null)
            {
                return result;
            }

            for (var idIndex = 0; idIndex < ids.Count; idIndex++)
            {
                var id = ids[idIndex];
                var parameterList = response.GdlLists[idIndex];

                if (parameterList.GdlParameterArray == null ||
                    parameterList.GdlParameterArray.Count == 0)
                {
                    continue;
                }

                result.AddRange(
                    parameterList
                        .GdlParameterArray.Where(x => x.Name == parameterName)
                        .Select(details => new GDLHolder(
                            id,
                            details)));
            }

            return result;
        }
    }
}