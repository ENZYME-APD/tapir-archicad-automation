using Grasshopper.Kernel.Types;
using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.Types.GuidObjects
{
    public abstract class GuidWrapper<I, T> : IFromGhWrapper
        where I : GuidObject<I>, new()
        where T : GuidWrapper<I, T>, new()
    {
        [JsonIgnore]
        public abstract I Id
        {
            get;
            set;
        }

        public static T CreateFromGhWrapper(
            GH_ObjectWrapper wrapper)
        {
            if (wrapper.Value is T)
            {
                return wrapper.Value as T;
            }
            else
            {
                var id = GuidObject<I>.CreateFromWrapper(wrapper);
                if (id != null)
                {
                    return new T { Id = id };
                }

                return null;
            }
        }

        public bool TryCreateFromGhWrapper(
            object wrapper)
        {
            T temporary = CreateFromGhWrapper((GH_ObjectWrapper)wrapper);

            if (temporary == null)
            {
                return false;
            }

            Id = temporary.Id;

            return true;
        }

        public override string ToString()
        {
            return Id?.ToString();
        }

        public override bool Equals(
            object other)
        {
            var o = other as GuidWrapper<I, T>;
            return o != null && Id.Guid == o.Id.Guid;
        }

        public override int GetHashCode()
        {
            return Id.GetHashCode();
        }
    }
}