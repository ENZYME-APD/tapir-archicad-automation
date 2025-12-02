using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using System.Collections.Generic;
using Grasshopper.Kernel.Types;
using TapirGrasshopperPlugin.ResponseTypes.IdObjects;

namespace TapirGrasshopperPlugin.Helps
{
    public static class ComponentHelp
    {
        public static bool TryGet<T>(
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

        public static bool TryGet<T>(
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

        public static bool TryGet<T>(
            this IGH_DataAccess da,
            int index,
            out GH_Structure<T> structure)
            where T : IGH_Goo
        {
            return da.GetDataTree(
                index,
                out structure);
        }

        public static T GetOptional<T>(
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

        public static bool TryCreate<T>(
            this IGH_DataAccess da,
            int index,
            out T result)
            where T : IFromGhWrapper, new()
        {
            if (da.TryGet(
                    index,
                    out GH_ObjectWrapper wrapper))
            {
                return da.Try(
                    index,
                    wrapper,
                    out result);
            }

            result = default;
            return false;
        }

        public static bool TryCreateFromList<T>(
            this IGH_DataAccess da,
            int index,
            out T result)
            where T : IFromGhWrapper, new()
        {
            if (da.TryGet(
                    index,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return da.Try(
                    index,
                    wrappers,
                    out result);
            }

            result = default;
            return false;
        }

        public static bool Try<T, T2>(
            this IGH_DataAccess da,
            int index,
            T2 wrapper,
            out T result)
            where T : IFromGhWrapper, new()
        {
            var instance = new T();

            if (!instance.TryCreateFromGhWrapper(wrapper))
            {
                result = default;
                return false;
            }

            result = instance;
            return true;
        }
    }
}