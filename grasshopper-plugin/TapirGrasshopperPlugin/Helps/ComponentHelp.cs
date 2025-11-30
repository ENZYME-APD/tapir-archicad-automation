using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using System.Collections.Generic;
using Grasshopper.Kernel.Types;

namespace TapirGrasshopperPlugin.Helps
{
    public static class ComponentHelp
    {
        public static bool TryGetItem<T>(
            this IGH_DataAccess da,
            int index,
            out T result)
        {
            T baseItem = default;

            var success = da.GetData(
                index,
                ref baseItem);

            result = baseItem;
            return success;
        }

        public static bool TryGetItems<T>(
            this IGH_DataAccess da,
            int index,
            out List<T> results)
        {
            var items = new List<T>();

            var success = da.GetDataList(
                index,
                items);

            results = items;
            return success;
        }

        public static bool TryGetTree<T>(
            this IGH_DataAccess da,
            int index,
            out GH_Structure<T> structure)
            where T : IGH_Goo
        {
            return da.GetDataTree(
                index,
                out structure);
        }

        public static T GetOptionalItem<T>(
            this IGH_DataAccess dataAccess,
            int index,
            T defItem)
        {
            var item = defItem;
            dataAccess.GetData(
                index,
                ref item);
            return item;
        }

        public static string AsString(
            this GH_ObjectWrapper wrapper)
        {
            if (wrapper.Value is GH_String)
            {
                return wrapper.Value.ToString();
            }
            else
            {
                throw new System.Exception(
                    $"Cannot convert {nameof(GH_ObjectWrapper)} to string.");
            }
        }

        public static void AddError(
            this GH_ActiveObject activeObject,
            string message)
        {
            activeObject.AddRuntimeMessage(
                GH_RuntimeMessageLevel.Error,
                message);
        }

        public static void AddWarning(
            this GH_ActiveObject active,
            string message)
        {
            active.AddRuntimeMessage(
                GH_RuntimeMessageLevel.Warning,
                message);
        }
    }
}