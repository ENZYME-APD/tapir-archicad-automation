using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // The LinkElements and UnlinkElements components take the two ends of
    // the links as parallel lists; this pairs them item by item.
    internal static class ElementLinkInputs
    {
        public static void Add(
            Component component)
        {
            component.InGenerics(
                "FromElementGuids",
                "Identifiers of the elements the links start from.");

            component.InGenerics(
                "ToElementGuids",
                "Identifiers of the elements the links point to, one for each item of FromElementGuids.");
        }

        // Reads the FromElementGuids and ToElementGuids inputs (at the given
        // indices) into one link per item. Returns null, with the reason
        // reported on the component, when the lists differ in size or hold
        // something that is not an element identifier.
        public static List<ElementLink> Read(
            Component component,
            IGH_DataAccess da,
            int fromIndex,
            int toIndex)
        {
            if (!da.TryGetList(
                    fromIndex,
                    out List<GH_ObjectWrapper> fromWrappers))
            {
                return null;
            }

            if (!da.TryGetList(
                    toIndex,
                    out List<GH_ObjectWrapper> toWrappers))
            {
                return null;
            }

            if (fromWrappers.Count != toWrappers.Count)
            {
                component.AddError(
                    "The size of the inputs FromElementGuids and ToElementGuids must be equal.");
                return null;
            }

            var links = new List<ElementLink>();

            for (var i = 0; i < fromWrappers.Count; i++)
            {
                var fromElementId = GuidObject<ElementGuid>.CreateFromWrapper(fromWrappers[i]);
                var toElementId = GuidObject<ElementGuid>.CreateFromWrapper(toWrappers[i]);
                if (fromElementId == null || toElementId == null)
                {
                    component.AddError(
                        "Invalid element identifier in the FromElementGuids or ToElementGuids input.");
                    return null;
                }

                links.Add(
                    new ElementLink
                    {
                        FromElementId = fromElementId,
                        ToElementId = toElementId
                    });
            }

            return links;
        }
    }
}
