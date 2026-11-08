using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    // The server-side element filter of #/ElementTypeFloorLayerFilter, taken by
    // GetDetailsOfElements and ApplyFavoritesToElements: an element matches when
    // its type, home story and layer are each among the listed ones; an omitted
    // list does not filter.
    public class ElementTypeFloorLayerFilter
    {
        [JsonProperty(
            "types",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<string> Types;

        [JsonProperty(
            "floorIndices",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<int> FloorIndices;

        [JsonProperty(
            "layerIndices",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<int> LayerIndices;

        // The filter of the given lists (each empty or null when its input is
        // not connected), or null when none of them filters.
        public static ElementTypeFloorLayerFilter FromLists(
            List<string> types,
            List<int> floorIndices,
            List<int> layerIndices)
        {
            var filter = new ElementTypeFloorLayerFilter
            {
                Types = NullIfEmpty(types),
                FloorIndices = NullIfEmpty(floorIndices),
                LayerIndices = NullIfEmpty(layerIndices)
            };

            return filter.Types == null &&
                   filter.FloorIndices == null &&
                   filter.LayerIndices == null
                ? null
                : filter;
        }

        private static List<T> NullIfEmpty<T>(
            List<T> list)
        {
            return list == null || list.Count == 0 ? null : list;
        }
    }

    public class GetDetailsOfElementsParameters
    {
        [JsonProperty(
            "elements",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<ElementGuidWrapper> Elements;

        [JsonProperty(
            "filter",
            NullValueHandling = NullValueHandling.Ignore)]
        public ElementTypeFloorLayerFilter Filter;
    }
}
