using System;

namespace TapirGrasshopperPlugin.Helps
{
    public static class EnumHelp
    {
        public static T ToEnum<T>(
            string value)
            where T : struct
        {
            if (Enum.TryParse(
                    value,
                    true,
                    out T result))
            {
                return result;
            }

            throw new ArgumentException(
                $"Value '{value}' is not a valid constant for {typeof(T).Name}");
        }
    }
}