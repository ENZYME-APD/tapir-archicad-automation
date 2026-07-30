using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System.Collections.Generic;
using System.Drawing;

namespace TapirGrasshopperPlugin.Helps
{
    // Helpers for converting Tapir JSON responses into structured Grasshopper
    // outputs. Items of "XOrError" unions are detected via their "error"
    // field; data outputs stay aligned with the inputs by emitting nulls
    // (lists) or empty branches (trees) at error positions.
    public static class JsonOutputHelp
    {
        public static bool IsError(
            JToken item)
        {
            return item?["error"] != null;
        }

        public static string ErrorMessage(
            JToken item)
        {
            return item?["error"]?["message"]?.ToString() ?? "";
        }

        // Scalar value of a field, or null when the field is absent.
        public static object Scalar(
            JToken item,
            string key)
        {
            var token = item?[key];
            if (token == null || token.Type == JTokenType.Null)
            {
                return null;
            }
            return ((JValue)token).Value;
        }

        // Guid string of an id object like {"guid": "..."}.
        public static string GuidOf(
            JToken idObject)
        {
            return idObject?["guid"]?.ToString();
        }

        // Guid string of a wrapped id object like {"attributeId": {"guid": "..."}}.
        public static string WrappedGuidOf(
            JToken wrapper,
            string key)
        {
            return wrapper?[key]?["guid"]?.ToString();
        }

        // 2D or 3D coordinate object to a point; null when absent.
        public static object Point(
            JToken coordinate)
        {
            if (coordinate == null || coordinate.Type == JTokenType.Null)
            {
                return null;
            }
            return new Point3d(
                coordinate.Value<double?>("x") ?? 0.0,
                coordinate.Value<double?>("y") ?? 0.0,
                coordinate.Value<double?>("z") ?? 0.0);
        }

        // ColorRGB object with red/green/blue in [0..1] to a color; null when absent.
        public static object ColorOf(
            JToken colorObject)
        {
            if (colorObject == null || colorObject.Type == JTokenType.Null)
            {
                return null;
            }
            return Color.FromArgb(
                ClampChannel(colorObject.Value<double?>("red")),
                ClampChannel(colorObject.Value<double?>("green")),
                ClampChannel(colorObject.Value<double?>("blue")));
        }

        private static int ClampChannel(
            double? value)
        {
            var channel = (int)((value ?? 0.0) * 255.0 + 0.5);
            if (channel < 0)
            {
                return 0;
            }
            if (channel > 255)
            {
                return 255;
            }
            return channel;
        }

        // Items of the top-level response array; empty list when absent.
        public static List<JToken> Items(
            JObject response,
            string arrayKey)
        {
            var result = new List<JToken>();
            if (response?[arrayKey] is JArray array)
            {
                foreach (var item in array)
                {
                    result.Add(item);
                }
            }
            return result;
        }
    }
}
