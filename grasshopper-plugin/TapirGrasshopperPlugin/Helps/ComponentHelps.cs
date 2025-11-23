using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System.Collections.Generic;
using Newtonsoft.Json.Linq;

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

        public static bool TryCreate<T>(
            this IGH_DataAccess da,
            int index,
            out T result)
            where T : class

        {
            if (da.TryGetItem(
                    index,
                    out GH_ObjectWrapper wrapper))
            {
                result = JObject.FromObject(wrapper.Value).ToObject<T>();
                return true;
            }

            result = null;
            return false;
        }

        //public static T Parse<T>(
        //    this GH_ObjectWrapper wrapper)
        //    where T : class
        //{
        //    if (wrapper.Value is T)
        //    {
        //        return wrapper.Value as T;
        //    }
        //    else if (wrapper.Value is GH_String)
        //    {
        //        var stringValue = wrapper.Value as GH_String;
        //        return FromString(stringValue.ToString());
        //    }
        //    else if (wrapper.Value.GetType().GetProperty("Guid") != null)
        //    {
        //        return FromString(
        //            wrapper.Value.GetType().GetProperty("Guid").ToString());
        //    }
        //    else
        //    {
        //        return null;
        //    }
        //}

        //public static T FromString(
        //    string guidString)
        //{
        //    if (System.Guid.TryParse(
        //            guidString,
        //            out _))
        //    {
        //        return new T { Guid = guidString };
        //    }
        //    else
        //    {
        //        return null;
        //    }
        //}
    }
}