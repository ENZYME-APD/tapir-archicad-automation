using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    // Shared code of the components of the analytical members, releases,
    // supports, links and loads. Their settings are large, version dependent
    // objects, so they are handed out and taken as JSON texts in the form the
    // commands return them.
    internal static class AnalyticalJsonHelp
    {
        public const string ModelGuidDescription =
            "Identifier of the Structural Analytical Model (from GetCurrentAnalyticalModel or GetAnalyticalModel). " +
            "Optional; the model of the current view is used when it is omitted. Required in Archicad 26.";

        public const string MemberPartRefDescription =
            "An analytical member part reference as JSON, for example " +
            "{\"elemGuid\": {\"guid\": \"...\"}, \"partRefType\": \"BegVertex\"}; partRefType is BegVertex, " +
            "EndVertex, CustomVertex, Edge, RatioEdge, Surface, CustomSurface or CustomEdge, with partId, " +
            "customVertexData or ratioEdgeData where the type needs them.";

        // The keys of the three kinds of analytical loads, with their names.
        public static readonly (string Key, string Name)[] LoadKinds =
        {
            ("pointLoad", "PointLoad"),
            ("edgeLoad", "EdgeLoad"),
            ("surfaceLoad", "SurfaceLoad")
        };

        // The keys of the three kinds of analytical members, with their names.
        public static readonly (string Key, string Name)[] MemberKinds =
        {
            ("curveMember", "CurveMember"),
            ("surfaceMember", "SurfaceMember"),
            ("segmentedSurfaceMembers", "SegmentedSurfaceMembers")
        };

        // Adds the identifier of the optional AnalyticalModelGuid input as
        // analyticalModelId. Returns false (after reporting an error) when the
        // identifier is invalid.
        public static bool TryAddModelId(
            GH_ActiveObject component,
            IGH_DataAccess da,
            int index,
            JObject parameters)
        {
            if (!da.TryGet(
                    index,
                    out GH_ObjectWrapper wrapper))
            {
                return true;
            }

            var modelId = GuidObject<AnalyticalModelGuid>.CreateFromWrapper(wrapper);
            if (modelId == null)
            {
                component.AddError("Invalid identifier in the AnalyticalModelGuid input.");
                return false;
            }
            parameters["analyticalModelId"] = AnalyticalHelp.IdOf(modelId);
            return true;
        }

        // Reads a required list of elements into parameters["elements"].
        public static bool TryAddElements(
            IGH_DataAccess da,
            int index,
            JObject parameters)
        {
            if (!da.TryCreateFromList(
                    index,
                    out ElementsObject elements))
            {
                return false;
            }
            parameters["elements"] = JObject.FromObject(elements)["elements"];
            return true;
        }

        // Parses a list of JSON object texts. Returns false (after reporting
        // an error) when one of them is not a JSON object.
        public static bool TryParseObjects(
            GH_ActiveObject component,
            List<string> texts,
            string inputName,
            out List<JObject> objects)
        {
            objects = new List<JObject>();
            for (var i = 0; i < texts.Count; i++)
            {
                if (!TryParseObject(component, texts[i], inputName, i, out JObject parsed))
                {
                    return false;
                }
                objects.Add(parsed);
            }
            return true;
        }

        public static bool TryParseObject(
            GH_ActiveObject component,
            string text,
            string inputName,
            int index,
            out JObject parsed)
        {
            parsed = null;
            try
            {
                parsed = JObject.Parse(text ?? "");
                return true;
            }
            catch (JsonException exception)
            {
                component.AddError(
                    $"Item {index} of the input {inputName} is not a JSON object: {exception.Message}");
                return false;
            }
        }

        // The object itself, or the value of wrapperKey when the object is an
        // item as a Get command returns it ({"support": {...}, "geometry": ...}).
        public static JObject Unwrap(
            JObject item,
            string wrapperKey)
        {
            return item[wrapperKey] is JObject inner ? inner : item;
        }

        // A copy of the object with only the given keys, so that an item as a
        // Get command returns it (with its geometry or connections) can be fed
        // back to the matching Create command.
        public static JObject Pick(
            JObject item,
            params string[] keys)
        {
            var picked = new JObject();
            foreach (var key in keys)
            {
                if (item[key] != null)
                {
                    picked[key] = item[key];
                }
            }
            return picked;
        }

        // The JSON text of a token; null when it is absent.
        public static string TextOf(
            JToken token)
        {
            if (token == null || token.Type == JTokenType.Null)
            {
                return null;
            }
            return token.ToString(Formatting.Indented);
        }

        // A coordinate or vector object as a vector; null when it is absent.
        public static object VectorOf(
            JToken coordinate)
        {
            if (coordinate == null || coordinate.Type == JTokenType.Null)
            {
                return null;
            }
            return new Vector3d(
                coordinate.Value<double?>("x") ?? 0.0,
                coordinate.Value<double?>("y") ?? 0.0,
                coordinate.Value<double?>("z") ?? 0.0);
        }

        // The element identifiers in an Elements array of the response.
        public static List<ElementGuidWrapper> ElementsOf(
            JToken elements)
        {
            return elements is JArray array
                ? array.ToObject<List<ElementGuidWrapper>>()
                : new List<ElementGuidWrapper>();
        }

        // Splits the items of an "XOrError" response array: the error message
        // of each item (empty when it is no error) and the items that are no
        // error (null at the positions of the errors).
        public static List<JToken> SplitErrors(
            JObject response,
            string arrayKey,
            out List<string> errors)
        {
            var items = new List<JToken>();
            errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(response, arrayKey))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    items.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                }
                else
                {
                    items.Add(item);
                    errors.Add("");
                }
            }
            return items;
        }

        // The JSON text of the given field of each item; null for errors.
        public static List<string> TextsOf(
            List<JToken> items,
            string key)
        {
            var texts = new List<string>();
            foreach (var item in items)
            {
                texts.Add(item == null ? null : TextOf(item[key]));
            }
            return texts;
        }
    }
}
