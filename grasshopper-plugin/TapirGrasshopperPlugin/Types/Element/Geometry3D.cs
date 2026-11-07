using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    // The response of Get3DGeometryOfElements: the solid bodies of the 3D
    // model of each element, or an error per element.

    public class Polygon3DHole
    {
        [JsonProperty("vertexIds")]
        public List<int> VertexIds;
    }

    public class Polygon3D
    {
        // The outer contour: 0 based indices into the body's vertices.
        [JsonProperty("vertexIds")]
        public List<int> VertexIds;

        [JsonProperty(
            "holes",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<Polygon3DHole> Holes;

        // Absent when the face's material is not a project attribute.
        [JsonProperty(
            "surfaceId",
            NullValueHandling = NullValueHandling.Ignore)]
        public AttributeGuidObject SurfaceId;
    }

    public class Body3D
    {
        // World coordinates.
        [JsonProperty("vertices")]
        public List<Point3D> Vertices;

        [JsonProperty("polygons")]
        public List<Polygon3D> Polygons;
    }

    public class Geometry3DItem
    {
        [JsonProperty(
            "bodies",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<Body3D> Bodies;

        [JsonProperty(
            "error",
            NullValueHandling = NullValueHandling.Ignore)]
        public Error Error;
    }

    public class Geometries3DObject
    {
        [JsonProperty("geometries3D")]
        public List<Geometry3DItem> Geometries3D;
    }
}
