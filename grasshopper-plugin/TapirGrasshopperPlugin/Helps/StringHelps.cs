using System;

namespace TapirGrasshopperPlugin.Helps
{
    public static class StringHelps
    {
        public static string WithTypeName(
            this string description,
            string type)
        {
            return
                $"[{type}]{Environment.NewLine}{Environment.NewLine}{description}";
        }
    }
}