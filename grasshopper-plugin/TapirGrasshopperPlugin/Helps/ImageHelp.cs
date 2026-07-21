using System;
using System.Drawing;
using System.IO;

namespace TapirGrasshopperPlugin.Helps
{
    public static class ImageHelp
    {
        // System.Drawing.Common is flagged by the analyzer as Windows-only
        // (CA1416), but Rhino ships a working System.Drawing implementation on
        // every platform it supports (the project references the package with
        // ExcludeAssets="runtime" and uses Rhino's copy at runtime) - the same
        // mechanism that renders the component icons on macOS. The runtime
        // fallback below covers any environment where the drawing backend is
        // genuinely unavailable: the method returns null and callers keep
        // working with the base64 output.
#pragma warning disable CA1416
        public static Bitmap FromBase64(
            string base64)
        {
            if (string.IsNullOrEmpty(base64))
            {
                return null;
            }

            try
            {
                var bytes = System.Convert.FromBase64String(base64);
                using (var stream = new MemoryStream(bytes))
                {
                    return new Bitmap(stream);
                }
            }
            catch (Exception ex) when (
                ex is PlatformNotSupportedException ||
                ex is TypeInitializationException ||
                ex is DllNotFoundException)
            {
                // No usable System.Drawing backend on this platform.
                return null;
            }
        }
#pragma warning restore CA1416
    }
}
