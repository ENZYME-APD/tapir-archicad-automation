using Grasshopper.Kernel;
using Grasshopper.Kernel.Parameters;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    // Shared by the navigator components: the "input only 1 to use the same
    // value for all" lists, the JSON object inputs checked against the keys a
    // command accepts, and the conversions of boxes and identifiers.
    public static class NavigatorComponentHelp
    {
        // The value for the item at index of a list that holds 0 (null),
        // 1 (the same for all) or one value per item.
        public static T GetAt<T>(
            List<T> values,
            int index)
        {
            if (values == null || values.Count == 0)
            {
                return default;
            }

            return values[values.Count == 1 ? 0 : index];
        }

        // Checks that a list input holds 0, 1 or itemCount values.
        public static bool CheckCount(
            GH_ActiveObject component,
            string inputName,
            int count,
            string itemsName,
            int itemCount)
        {
            if (count > 1 && count != itemCount)
            {
                component.AddError(
                    $"The size of the input {inputName} must be 0, 1 or equal to the size of the input {itemsName}.");
                return false;
            }

            return true;
        }

        // Parses a JSON object input and checks that it only holds the keys
        // the command accepts, so a typo is reported instead of being ignored
        // or refused by the command with a less clear message.
        public static bool TryParseObject(
            GH_ActiveObject component,
            string inputName,
            string text,
            ICollection<string> allowedKeys,
            out JObject result)
        {
            result = null;

            if (string.IsNullOrWhiteSpace(text))
            {
                result = new JObject();
                return true;
            }

            try
            {
                result = JObject.Parse(text);
            }
            catch (JsonReaderException ex)
            {
                component.AddError(
                    $"The input {inputName} is not a valid JSON object: {ex.Message}");
                return false;
            }

            var unknownKeys = result
                .Properties()
                .Select(p => p.Name)
                .Where(name => !allowedKeys.Contains(name))
                .ToList();

            if (unknownKeys.Count > 0)
            {
                component.AddError(
                    $"Unknown keys in the input {inputName}: {string.Join(", ", unknownKeys)}. " +
                    $"Accepted keys: {string.Join(", ", allowedKeys)}.");
                result = null;
                return false;
            }

            return true;
        }

        // Optional item inputs without a default value: an input with one always
        // supplies data, which would send a key the user did not give.
        public static void AddOptionalBoolean(
            GH_Component component,
            string name,
            string description)
        {
            AddOptional(
                component,
                new Param_Boolean(),
                name,
                description.WithTypeName("booleanItem"));
        }

        public static void AddOptionalInteger(
            GH_Component component,
            string name,
            string description)
        {
            AddOptional(
                component,
                new Param_Integer(),
                name,
                description.WithTypeName("integerItem"));
        }

        public static void AddOptionalNumber(
            GH_Component component,
            string name,
            string description)
        {
            AddOptional(
                component,
                new Param_Number(),
                name,
                description.WithTypeName("numberItem"));
        }

        public static void AddOptionalText(
            GH_Component component,
            string name,
            string description)
        {
            AddOptional(
                component,
                new Param_String(),
                name,
                description.WithTypeName("textItem"));
        }

        private static void AddOptional(
            GH_Component component,
            IGH_Param param,
            string name,
            string description)
        {
            param.Name = name;
            param.NickName = name;
            param.Description = description;
            param.Access = GH_ParamAccess.item;
            param.Optional = true;
            component.Params.RegisterInputParam(param);
        }

        // An item output of the given parameter type.
        public static void AddOutput(
            GH_Component component,
            IGH_Param param,
            string name,
            string description)
        {
            param.Name = name;
            param.NickName = name;
            param.Description = description;
            param.Access = GH_ParamAccess.item;
            component.Params.RegisterOutputParam(param);
        }

        // Copies an optional item input to the key of parameters when it was given.
        public static void CopyOptional<T>(
            IGH_DataAccess da,
            int index,
            JObject parameters,
            string key)
        {
            var value = default(T);
            if (da.GetData(
                    index,
                    ref value) &&
                value != null)
            {
                parameters[key] = JToken.FromObject(value);
            }
        }

        // An identifier object like {"guid": "..."}.
        public static JObject IdObject(
            string guid)
        {
            return new JObject { ["guid"] = guid };
        }

        // A BoundingBox2D or Zoom object to a rectangle on the XY plane; null
        // when absent.
        public static object Rectangle(
            JToken box)
        {
            if (box == null || box.Type == JTokenType.Null)
            {
                return null;
            }

            return new Rectangle3d(
                Plane.WorldXY,
                new Point3d(
                    box.Value<double?>("xMin") ?? 0.0,
                    box.Value<double?>("yMin") ?? 0.0,
                    0.0),
                new Point3d(
                    box.Value<double?>("xMax") ?? 0.0,
                    box.Value<double?>("yMax") ?? 0.0,
                    0.0));
        }

        // The JSON text of an object output, or null when absent.
        public static string JsonText(
            JToken token)
        {
            if (token == null || token.Type == JTokenType.Null)
            {
                return null;
            }

            return token.ToString(Formatting.None);
        }
    }
}
