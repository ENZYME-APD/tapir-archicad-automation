using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    // Shared input and output handling of ReindexLayers and ReindexAttributes.
    public static class ReindexOutputHelp
    {
        // The {"attributeId": ..., "newIndex": ...} items of the moves; null
        // (with an error) when the inputs do not match.
        public static JArray MovesOf(
            GH_ActiveObject owner,
            List<GH_ObjectWrapper> attributeWrappers,
            List<int> newIndices,
            string attributesInputName)
        {
            if (newIndices.Count != attributeWrappers.Count)
            {
                owner.AddError(
                    $"The size of the input NewIndices must be equal to the size of the input {attributesInputName}.");
                return null;
            }

            var moves = new JArray();
            for (var i = 0; i < attributeWrappers.Count; i++)
            {
                var id = GuidObject<AttributeGuidObject>.CreateFromWrapper(attributeWrappers[i]);
                if (id == null)
                {
                    owner.AddError($"Invalid attribute identifier in the {attributesInputName} input.");
                    return null;
                }

                moves.Add(
                    new JObject
                    {
                        ["attributeId"] = new JObject { ["guid"] = id.Guid },
                        ["newIndex"] = newIndices[i]
                    });
            }

            return moves;
        }

        public static AttributeGuidWrapper AttributeGuidOf(
            JToken item)
        {
            var guid = JsonOutputHelp.WrappedGuidOf(
                item,
                "attributeId");
            return guid == null
                ? null
                : new AttributeGuidWrapper
                {
                    AttributeId = new AttributeGuidObject { Guid = guid }
                };
        }

        // The elements of an Elements array ({"elementId": ...} items) into a branch.
        public static void AddElements(
            DataTree<object> tree,
            JToken elements,
            GH_Path path)
        {
            tree.EnsurePath(path);
            if (elements is JArray items)
            {
                foreach (var item in items)
                {
                    tree.Add(
                        ItemListHelp.ElementGuidOf(item?["elementId"]),
                        path);
                }
            }
        }

        // The strings of an array into a branch.
        public static void AddTexts(
            DataTree<string> tree,
            JToken texts,
            GH_Path path)
        {
            tree.EnsurePath(path);
            if (texts is JArray items)
            {
                foreach (var item in items)
                {
                    tree.Add(
                        item?.ToString(),
                        path);
                }
            }
        }
    }
}
