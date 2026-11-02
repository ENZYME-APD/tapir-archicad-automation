using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Helps
{
    // Input helpers of the per-item element components.
    public static class ElementInputHelp
    {
        // Checks that a list input holds 1 item (used for all) or as many
        // items as there are elements; adds an error otherwise.
        public static bool CheckOneOrMany<T>(
            GH_ActiveObject component,
            List<T> values,
            int count,
            string inputName)
        {
            if (values.Count == 1 || values.Count == count)
            {
                return true;
            }

            component.AddError(
                $"The input {inputName} must hold 1 item (used for all) or {count} items, " +
                $"not {values.Count}.");
            return false;
        }

        // The value for the given item of a list checked by CheckOneOrMany.
        public static T ItemAt<T>(
            List<T> values,
            int index)
        {
            return values[values.Count == 1 ? 0 : index];
        }

        // Element identifiers of a list input; adds an error and returns null
        // when one of them is not an element identifier.
        public static List<ElementGuid> ElementGuids(
            GH_ActiveObject component,
            List<GH_ObjectWrapper> wrappers,
            string inputName)
        {
            var ids = new List<ElementGuid>();
            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    component.AddError(
                        $"Invalid element identifier in the {inputName} input.");
                    return null;
                }
                ids.Add(id);
            }
            return ids;
        }
    }
}
