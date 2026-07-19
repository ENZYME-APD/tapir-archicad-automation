using System;
using System.Drawing;
using System.IO;

namespace TapirGrasshopperPlugin.Helps
{
    public static class ImageHelp
    {
        public static Bitmap FromBase64(
            string base64)
        {
            if (string.IsNullOrEmpty(base64))
            {
                return null;
            }

            var bytes = System.Convert.FromBase64String(base64);
            using (var stream = new MemoryStream(bytes))
            {
                return new Bitmap(stream);
            }
        }
    }
}
