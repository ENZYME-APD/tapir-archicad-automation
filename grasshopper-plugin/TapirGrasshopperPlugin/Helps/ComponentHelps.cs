using Grasshopper.Kernel;

namespace TapirGrasshopperPlugin.Helps
{
    public static class ComponentHelps
    {
        public static bool GetItem<T> (this IGH_DataAccess dataAccess, int index, out T result)
        {
            T baseItem = default;
            var success = dataAccess.GetData (index, ref baseItem);
            result = baseItem;
            return success;
        }
    }
}
