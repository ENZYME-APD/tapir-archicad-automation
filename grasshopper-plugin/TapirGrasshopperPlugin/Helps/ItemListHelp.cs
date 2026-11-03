using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Helps
{
    // Helpers for the components whose list inputs follow the "nothing, one
    // value for all items or one value per item" convention.
    public static class ItemListHelp
    {
        // True when every list holds 0, 1 or count items; adds an error naming
        // the first list that does not.
        public static bool CheckSizes(
            this GH_ActiveObject owner,
            int count,
            string countInputName,
            params (string Name, int Count)[] lists)
        {
            foreach (var list in lists)
            {
                if (list.Count > 1 &&
                    list.Count != count)
                {
                    owner.AddError(
                        $"The size of the input {list.Name} must be 0, 1 or equal to the size of the input {countInputName}.");
                    return false;
                }
            }

            return true;
        }

        // The item of a list for item i: its only item, or its i-th one.
        public static T At<T>(
            this List<T> list,
            int i)
        {
            return list.Count == 1 ? list[0] : list[i];
        }

        // Reads an optional list input; an empty list when nothing is given.
        public static List<T> OptionalList<T>(
            this IGH_DataAccess da,
            int index)
        {
            var items = new List<T>();
            da.GetDataList(
                index,
                items);
            return items;
        }

        // The {"guid": ...} objects of an element guid list input; false (with
        // an error) when one of them is not an element identifier.
        public static bool TryGetElementIds(
            this GH_ActiveObject owner,
            List<GH_ObjectWrapper> wrappers,
            string inputName,
            out List<JObject> elementIds)
        {
            elementIds = new List<JObject>();
            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    owner.AddError($"Invalid element identifier in the {inputName} input.");
                    return false;
                }

                elementIds.Add(new JObject { ["guid"] = id.Guid });
            }

            return true;
        }

        // The element guid wrapper of an {"guid": ...} object; null when absent.
        public static ElementGuidWrapper ElementGuidOf(
            JToken elementId)
        {
            var guid = elementId?["guid"]?.ToString();
            return guid == null
                ? null
                : new ElementGuidWrapper { ElementId = new ElementGuid { Guid = guid } };
        }

        // Some commands answer with a single execution result
        // ({"success": ..., "error": ...}) instead of a list of them. Adds the
        // error of a failed one; true when it succeeded.
        public static bool CheckExecutionResult(
            this GH_ActiveObject owner,
            JObject response)
        {
            if (response == null ||
                response["success"] == null)
            {
                return true;
            }

            var result = ExecutionResult.Deserialize(response);
            if (!result.Success)
            {
                owner.AddError(result.Message());
                return false;
            }

            return true;
        }
    }
}
