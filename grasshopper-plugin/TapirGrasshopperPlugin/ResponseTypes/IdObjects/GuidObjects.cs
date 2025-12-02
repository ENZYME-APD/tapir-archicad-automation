using Grasshopper.Kernel.Types;
using Grasshopper.Kernel;
using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.ResponseTypes.IdObjects
{
    public interface IFromGhWrapper
    {
        bool TryCreateFromWrapper(
            object wrapper);
    }

    public abstract class GuidObject<T> : IFromGhWrapper
        where T : GuidObject<T>, new()
    {
        [JsonProperty("guid")]
        public string Guid;

        public static T CreateFromWrapper(
            GH_ObjectWrapper wrapper)
        {
            if (wrapper == null || wrapper.Value == null)
            {
                return null;
            }

            else if (wrapper.Value is T value)
            {
                return value;
            }

            else if (wrapper.Value is GH_String ghString)
            {
                return CreateFromString(ghString.Value);
            }

            else if (wrapper.Value is string rawString)
            {
                return CreateFromString(rawString);
            }

            else
            {
                var propInfo = wrapper.Value.GetType().GetProperty("Guid");

                var propInfoTest =
                    wrapper.Value.GetType().GetProperty("GuidItem");


                if (propInfo == null)
                {
                    return null;
                }

                var propValue = propInfo
                    .GetValue(
                        wrapper.Value,
                        null)
                    ?.ToString();

                if (!string.IsNullOrEmpty(propValue))
                {
                    return CreateFromString(propValue);
                }

                return null;
            }
        }

        public static T CreateFromString(
            string guidString)
        {
            if (string.IsNullOrWhiteSpace(guidString)) return null;

            if (System.Guid.TryParse(
                    guidString,
                    out _))
            {
                return new T { Guid = guidString };
            }

            return null;
        }

        public override string ToString()
        {
            return Guid;
        }

        public override bool Equals(
            object other)
        {
            var o = other as GuidObject<T>;
            return o != null && Guid == o.Guid;
        }

        public override int GetHashCode()
        {
            return Guid.GetHashCode();
        }

        public bool IsNullGuid()
        {
            return Guid == null || new System.Guid(Guid) == System.Guid.Empty;
        }

        public bool TryCreateFromWrapper(
            object wrapper)
        {
            T temporary = CreateFromWrapper((GH_ObjectWrapper)wrapper);

            if (temporary == null)
            {
                return false;
            }

            Guid = temporary.Guid;
            return true;
        }
    }

    public abstract class GuidItemObject<I, T> : IFromGhWrapper
        where I : GuidObject<I>, new()
        where T : GuidItemObject<I, T>, new()
    {
        [JsonIgnore]
        public abstract I Id
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
                    out GH_ObjectWrapper wrapper))
            {
                return CreateFromWrapper(wrapper);
            }

            return null;
        }

        public static T CreateFromWrapper(
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

        public bool TryCreateFromWrapper(
            object wrapper)
        {
            T temporary = CreateFromWrapper((GH_ObjectWrapper)wrapper);

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
            var o = other as GuidItemObject<I, T>;
            return o != null && Id.Guid == o.Id.Guid;
        }

        public override int GetHashCode()
        {
            return Id.GetHashCode();
        }
    }

    public abstract class GuidItemsObject<I, J, T> : IFromGhWrapper
        where I : GuidObject<I>, new()
        where J : GuidItemObject<I, J>, new()
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
                var item = GuidItemObject<I, J>.CreateFromWrapper(wrapper);

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

        public bool TryCreateFromWrapper(
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