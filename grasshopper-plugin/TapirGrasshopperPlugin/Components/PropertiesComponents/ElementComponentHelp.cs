using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    // Shared by the element component commands. A component of an element (a
    // building material skin or a profile part) is identified by its element
    // and its component guid together, as GetComponentsOfElements returns them
    // in its ElementGuids and ComponentGuids outputs.
    public static class ElementComponentHelp
    {
        public const string ElementGuidsDescription =
            "The element of each component, as GetComponentsOfElements returns it. Input only 1 when all " +
            "components belong to the same element.";

        public const string ComponentGuidsDescription =
            "The components to query, as GetComponentsOfElements returns them.";

        // The ElementComponents array ([{ "elementComponentId": { "elementId",
        // "componentId" } }]) of an element guid and a component guid list
        // input; false (with an error) when they are invalid.
        public static bool TryGetElementComponents(
            GH_ActiveObject owner,
            IGH_DataAccess da,
            int elementGuidsIndex,
            int componentGuidsIndex,
            out JArray elementComponents)
        {
            elementComponents = null;

            if (!da.TryGetList(
                    elementGuidsIndex,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !da.TryGetList(
                    componentGuidsIndex,
                    out List<string> componentGuids))
            {
                return false;
            }

            var elementIds = ElementInputHelp.ElementGuids(
                owner,
                elementWrappers,
                "ElementGuids");
            if (elementIds == null)
            {
                return false;
            }

            if (componentGuids.Count == 0)
            {
                owner.AddError("The input ComponentGuids is empty.");
                return false;
            }

            if (!ElementInputHelp.CheckOneOrMany(
                    owner,
                    elementIds,
                    componentGuids.Count,
                    "ElementGuids"))
            {
                return false;
            }

            elementComponents = new JArray();
            for (var i = 0; i < componentGuids.Count; i++)
            {
                if (string.IsNullOrWhiteSpace(componentGuids[i]))
                {
                    owner.AddError("The input ComponentGuids holds an empty item.");
                    elementComponents = null;
                    return false;
                }

                elementComponents.Add(
                    new JObject
                    {
                        ["elementComponentId"] = new JObject
                        {
                            ["elementId"] = new JObject
                            {
                                ["guid"] = ElementInputHelp.ItemAt(elementIds, i).Guid
                            },
                            ["componentId"] = new JObject { ["guid"] = componentGuids[i].Trim() }
                        }
                    });
            }

            return true;
        }
    }
}
