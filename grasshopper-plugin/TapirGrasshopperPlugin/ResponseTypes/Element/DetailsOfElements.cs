using Newtonsoft.Json;
using Rhino.Geometry;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class Point2D
    {
        public static Point2D Create(
            object obj)
        {
            if (obj is Point2D)
            {
                return obj as Point2D;
            }
            else if (obj is Point2d)
            {
                var point2D = (Point2d)obj;
                return new Point2D() { X = point2D.X, Y = point2D.Y };
            }
            else if (obj is Point3d)
            {
                var point3D = (Point3d)obj;
                return new Point2D() { X = point3D.X, Y = point3D.Y };
            }
            else
            {
                return null;
            }
        }

        [JsonProperty("x")]
        public double X;

        [JsonProperty("y")]
        public double Y;
    }

    public class Point3D
    {
        public static Point3D Create(
            object obj)
        {
            if (obj is Point3D)
            {
                return obj as Point3D;
            }
            else if (obj is Point3d)
            {
                var point3D = (Point3d)obj;
                return new Point3D()
                {
                    X = point3D.X, Y = point3D.Y, Z = point3D.Z
                };
            }
            else
            {
                return null;
            }
        }

        [JsonProperty("x")]
        public double X;

        [JsonProperty("y")]
        public double Y;

        [JsonProperty("z")]
        public double Z;
    }

    public class Arc
    {
        [JsonProperty("begIndex")]
        public int BegIndex;

        [JsonProperty("endIndex")]
        public int EndIndex;

        [JsonProperty("arcAngle")]
        public double arcAngle;
    }

    public class WallDetails
    {
        [JsonProperty("geometryType")]
        public string GeometryType;

        [JsonProperty("zCoordinate")]
        public double ZCoordinate;

        [JsonProperty("begCoordinate")]
        public Point2D BegCoordinate;

        [JsonProperty("endCoordinate")]
        public Point2D EndCoordinate;

        [JsonProperty("height")]
        public double Height;

        [JsonProperty(
            "arcAngle",
            NullValueHandling = NullValueHandling.Ignore)]
        public double? ArcAngle;
    }

    public class BeamDetails
    {
        [JsonProperty("zCoordinate")]
        public double ZCoordinate;

        [JsonProperty("begCoordinate")]
        public Point2D BegCoordinate;

        [JsonProperty("endCoordinate")]
        public Point2D EndCoordinate;

        [JsonProperty("level")]
        public double Level;

        [JsonProperty("offset")]
        public double Offset;

        [JsonProperty("slantAngle")]
        public double SlantAngle;

        [JsonProperty("arcAngle")]
        public double ArcAngle;

        [JsonProperty("verticalCurveHeight")]
        public double VerticalCurveHeight;
    }

    public class ColumnDetails
    {
        [JsonProperty("origin")]
        public Point2D OrigoCoordinate;

        [JsonProperty("zCoordinate")]
        public double ZCoordinate;
    }

    public class HoleDetails
    {
        [JsonProperty("polygonOutline")]
        public List<Point2D> PolygonCoordinates;

        [JsonProperty(
            "polygonArcs",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<Arc> PolygonArcs;
    }

    public class SlabDetails
    {
        [JsonProperty("zCoordinate")]
        public double ZCoordinate;

        [JsonProperty("polygonOutline")]
        public List<Point2D> PolygonCoordinates;

        [JsonProperty(
            "polygonArcs",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<Arc> PolygonArcs;

        [JsonProperty("holes")]
        public List<HoleDetails> Holes;
    }

    public class PolylineDetails
    {
        [JsonProperty("coordinates")]
        public List<Point2D> Coordinates;

        [JsonProperty(
            "arcs",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<Arc> Arcs;

        [JsonProperty("zCoordinate")]
        public double ZCoordinate;
    }

    public class ZoneDetails
    {
        [JsonProperty("name")]
        public string Name;

        [JsonProperty("numberStr")]
        public string NumberStr;

        [JsonProperty("categoryAttributeId")]
        public AttributeGuidObject CategoryAttributeId;

        [JsonProperty("stampPosition")]
        public Point2D StampPosition;

        [JsonProperty("isManual")]
        public bool IsManual;

        [JsonProperty("zCoordinate")]
        public double ZCoordinate;

        [JsonProperty("polygonOutline")]
        public List<Point2D> PolygonCoordinates;

        [JsonProperty(
            "polygonArcs",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<Arc> PolygonArcs;

        [JsonProperty("holes")]
        public List<HoleDetails> Holes;
    }
}