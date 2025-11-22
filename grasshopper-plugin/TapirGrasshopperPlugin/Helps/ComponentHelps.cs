using Grasshopper.Kernel;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Helps
{
    public static class ComponentHelps
    {
        public static bool TryGetItem<T>(
            this IGH_DataAccess dataAccess,
            int index,
            out T result)
        {
            T baseItem = default;

            var success = dataAccess.GetData(
                index,
                ref baseItem);

            result = baseItem;
            return success;
        }

        public static bool GetItems<T>(
            this IGH_DataAccess dataAccess,
            int index,
            out List<T> results)
        {
            var items = new List<T>();

            var success = dataAccess.GetDataList(
                index,
                items);

            results = items;
            return success;
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
    }
}