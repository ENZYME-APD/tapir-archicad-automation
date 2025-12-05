using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.ResponseTypes.GuidObjects
{
    public abstract class GuidItemsObject<I, J, T> : IFromGhWrapper
        where I : GuidObject<I>, new()
        where J : GuidWrapper<I, J>, new()
        where T : GuidItemsObject<I, J, T>, new()
    {
        [JsonIgnore]
        public abstract List<J> GuidWrappers
        {
            get;
            set;
        }

        public static T Create(
            IGH_DataAccess da,
            int index)
        {
            if (da.TryGetList(
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
            var gObject = new T { GuidWrappers = new List<J>() };

            foreach (var wrapper in wrappers)
            {
                var item = GuidWrapper<I, J>.CreateFromGhWrapper(wrapper);

                if (item != null)
                {
                    gObject.GuidWrappers.Add(item);
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

            GuidWrappers = temporary.GuidWrappers;

            return true;
        }
    }
}