using System;

namespace TapirGrasshopperPlugin.Helps
{
    public static class StringHelp
    {
        public static string WithTypeName(
            this string description,
            string type)
        {
            return
                $"[{type}]{Environment.NewLine}{Environment.NewLine}{description}";
        }

        public static string Join(
            string text,
            string otherText)
        {
            return text + "/" + otherText;
        }
    }
}