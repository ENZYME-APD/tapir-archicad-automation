using Grasshopper.Kernel.Types;
using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.ResponseTypes.IdObjects
{
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

        public bool TryCreateFromGhWrapper(
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
}