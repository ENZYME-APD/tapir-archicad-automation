using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using System.Collections.Generic;
using Grasshopper.Kernel.Types;
using TapirGrasshopperPlugin.ResponseTypes.GuidObjects;

namespace TapirGrasshopperPlugin.Helps
{
    public static class ComponentHelp
    {
        public static GH_ObjectWrapper ToWrapper(
            this IGH_Goo goo)
        {
            return new GH_ObjectWrapper(goo);
        }

        public static bool EqualsTo<T1, T2>(
            this GH_Structure<T1> tree,
            GH_Structure<T2> other)
            where T1 : IGH_Goo
            where T2 : IGH_Goo

        {
            if (tree.Branches.Count != other.Branches.Count ||
                tree.PathCount != other.PathCount)
            {
                return false;
            }

            for (int i = 0; i < tree.Branches.Count; i++)
            {
                if (tree[i].Count != other[i].Count)
                {
                    return false;
                }
            }

            return true;
        }

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

        public static bool TryGetList<T>(
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


        public static List<T> GetOptionals<T>(
            this IGH_DataAccess dataAccess,
            int index)
        {
            var list = new List<T>();

            dataAccess.GetDataList(
                index,
                list);

            return list;
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
                return wrapper.TryBuildObject(out result);
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
            if (da.TryGetList(
                    index,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return wrappers.TryBuildObject(out result);
            }

            result = default;
            return false;
        }

        public static bool TryBuildObject<T, T2>(
            this T2 wrapper,
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