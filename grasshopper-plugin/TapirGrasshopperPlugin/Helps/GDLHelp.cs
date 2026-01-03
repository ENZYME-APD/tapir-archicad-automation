using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Helps
{
    public static class GDLParameterHelps
    {
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