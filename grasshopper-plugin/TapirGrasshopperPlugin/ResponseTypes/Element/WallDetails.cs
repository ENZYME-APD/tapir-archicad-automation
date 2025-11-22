using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class SetWallDetails
    {
        [JsonProperty("begCoordinate")]
        public Point2D BegCoordinate;

        [JsonProperty("endCoordinate")]
        public Point2D EndCoordinate;

        [JsonProperty("height")]
        public double Height;
    }

    public class TypedDetails<T>
    {
        [JsonProperty("typeSpecificDetails")]
        public T TypeSpecificDetails;
    }

    public class TypedElementWithDetailsObj<T>
    {
        [JsonProperty("elementId")]
        public ElementIdObj ElementId;

        [JsonProperty("details")]
        public TypedDetails<T> Details;
    }

    public class TypedElementsWithDetailsObj<T>
    {
        [JsonProperty("elementsWithDetails")]
        public List<TypedElementWithDetailsObj<T>> ElementsWithDetails;
    }
}