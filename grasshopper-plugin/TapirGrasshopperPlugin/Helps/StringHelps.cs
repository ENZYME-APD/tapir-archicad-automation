using System;

namespace TapirGrasshopperPlugin.Helps
{
    public static class StringHelps
    {
        public static string WithType(
            this string description,
            string type)
        {
            return
                $"[{type}]{Environment.NewLine}{Environment.NewLine}{description}";
        }
    }
}