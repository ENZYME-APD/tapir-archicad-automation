using Grasshopper.Kernel.Types;
using Grasshopper.Kernel;
using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.ResponseTypes.IdObjects
{
    public abstract class GuidItemsObject<I, J, T> : IFromGhWrapper
        where I : GuidObject<I>, new()
        where J : GuidWrapper<I, J>, new()
        where T : GuidItemsObject<I, J, T>, new()
    {
        [JsonIgnore]
        public abstract List<J> GuidItems
        {
            get;
            set;
        }

        public static T Create(
            IGH_DataAccess da,
            int index)
        {
            if (da.TryGet(
                    index,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return ConvertFrom(wrappers);
            }

            return null;
        }

        public static T ConvertFrom(
            List<GH_ObjectWrapper> wrappers)
        {
            var gObject = new T { GuidItems = new List<J>() };

            foreach (var wrapper in wrappers)
            {
                var item = GuidWrapper<I, J>.CreateFromGhWrapper(wrapper);

                if (item != null)
                {
                    gObject.GuidItems.Add(item);
                }
                else
                {
                    return null;
                }
            }

            return gObject;
        }

        public bool TryCreateFromGhWrapper(
            object wrappers)
        {
            T temporary = ConvertFrom((List<GH_ObjectWrapper>)wrappers);

            if (temporary == null)
            {
                return false;
            }

            GuidItems = temporary.GuidItems;

            return true;
        }
    }
}