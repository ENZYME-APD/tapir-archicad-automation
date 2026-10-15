using Newtonsoft.Json;
using Rhino.Geometry;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Types.Element
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

        // The fields below mirror the inputs of CreateWalls and ModifyWalls.

        [JsonProperty("begThickness")]
        public double BegThickness;

        [JsonProperty("offset")]
        public double Offset;

        [JsonProperty("bottomOffset")]
        public double BottomOffset;

        [JsonProperty("referenceLineLocation")]
        public string ReferenceLineLocation;

        [JsonProperty("structureType")]
        public string StructureType;

        // Only the one matching the structure type is present.
        [JsonProperty(
            "buildingMaterialId",
            NullValueHandling = NullValueHandling.Ignore)]
        public AttributeGuidObject BuildingMaterialId;

        [JsonProperty(
            "compositeId",
            NullValueHandling = NullValueHandling.Ignore)]
        public AttributeGuidObject CompositeId;

        [JsonProperty(
            "profileId",
            NullValueHandling = NullValueHandling.Ignore)]
        public AttributeGuidObject ProfileId;
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

        // The fields below mirror the inputs of CreateBeams. The section
        // fields come from the first segment of the beam and are absent when
        // the segments could not be read.

        [JsonProperty("anchorPoint")]
        public string AnchorPoint;

        [JsonProperty(
            "width",
            NullValueHandling = NullValueHandling.Ignore)]
        public double? Width;

        [JsonProperty(
            "height",
            NullValueHandling = NullValueHandling.Ignore)]
        public double? Height;

        [JsonProperty(
            "circleBased",
            NullValueHandling = NullValueHandling.Ignore)]
        public bool? CircleBased;

        // Only the one matching the structure of the segment is present.
        [JsonProperty(
            "buildingMaterialId",
            NullValueHandling = NullValueHandling.Ignore)]
        public AttributeGuidObject BuildingMaterialId;

        [JsonProperty(
            "profileId",
            NullValueHandling = NullValueHandling.Ignore)]
        public AttributeGuidObject ProfileId;
    }

    public class ColumnDetails
    {
        [JsonProperty("origin")]
        public Point2D OrigoCoordinate;

        [JsonProperty("zCoordinate")]
        public double ZCoordinate;

        // The fields below mirror the inputs of CreateColumns and
        // ModifyColumns. The section fields come from the first segment of
        // the column and are absent when the segments could not be read.

        [JsonProperty("height")]
        public double Height;

        [JsonProperty("bottomOffset")]
        public double BottomOffset;

        [JsonProperty("axisRotationAngle")]
        public double AxisRotationAngle;

        [JsonProperty("coreAnchor")]
        public string CoreAnchor;

        [JsonProperty(
            "width",
            NullValueHandling = NullValueHandling.Ignore)]
        public double? Width;

        [JsonProperty(
            "depth",
            NullValueHandling = NullValueHandling.Ignore)]
        public double? Depth;

        [JsonProperty(
            "circleBased",
            NullValueHandling = NullValueHandling.Ignore)]
        public bool? CircleBased;

        // Only the one matching the structure of the segment is present.
        [JsonProperty(
            "buildingMaterialId",
            NullValueHandling = NullValueHandling.Ignore)]
        public AttributeGuidObject BuildingMaterialId;

        [JsonProperty(
            "profileId",
            NullValueHandling = NullValueHandling.Ignore)]
        public AttributeGuidObject ProfileId;
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

        // The fields below mirror the inputs of CreateSlabs and ModifySlabs.

        [JsonProperty("thickness")]
        public double Thickness;

        [JsonProperty("structureType")]
        public string StructureType;

        [JsonProperty("referencePlaneLocation")]
        public string ReferencePlaneLocation;

        // Only the one matching the structure type is present.
        [JsonProperty(
            "buildingMaterialId",
            NullValueHandling = NullValueHandling.Ignore)]
        public AttributeGuidObject BuildingMaterialId;

        [JsonProperty(
            "compositeId",
            NullValueHandling = NullValueHandling.Ignore)]
        public AttributeGuidObject CompositeId;
    }

    public class RoofPivotLine
    {
        [JsonProperty("begin")]
        public Point2D Begin;

        [JsonProperty("end")]
        public Point2D End;
    }

    public class RoofLevel
    {
        [JsonProperty("height")]
        public double Height;

        [JsonProperty("angle")]
        public double Angle;
    }

    public class RoofDetails
    {
        [JsonProperty("roofClass")]
        public string RoofClass;

        [JsonProperty("structureType")]
        public string StructureType;

        [JsonProperty("thickness")]
        public double Thickness;

        [JsonProperty("level")]
        public double Level;

        [JsonProperty("zCoordinate")]
        public double ZCoordinate;

        [JsonProperty(
            "angle",
            NullValueHandling = NullValueHandling.Ignore)]
        public double? Angle;

        [JsonProperty(
            "pivotLine",
            NullValueHandling = NullValueHandling.Ignore)]
        public RoofPivotLine PivotLine;

        [JsonProperty(
            "eavesOverhang",
            NullValueHandling = NullValueHandling.Ignore)]
        public double? EavesOverhang;

        [JsonProperty(
            "levels",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<RoofLevel> Levels;

        [JsonProperty(
            "pivotPolygonOutline",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<Point2D> PivotPolygonOutline;

        [JsonProperty("polygonOutline")]
        public List<Point2D> PolygonCoordinates;

        [JsonProperty(
            "polygonArcs",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<Arc> PolygonArcs;

        [JsonProperty(
            "holes",
            NullValueHandling = NullValueHandling.Ignore)]
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

    public class LabelDetails
    {
        [JsonProperty(
            "text",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Text;
    }

    public class TextDetails
    {
        [JsonProperty("text")]
        public string Text;

        [JsonProperty("position")]
        public Point2D Position;

        [JsonProperty("angle")]
        public double Angle;

        [JsonProperty("height")]
        public double Height;

        [JsonProperty("pen")]
        public int Pen;

        [JsonProperty("justification")]
        public string Justification;

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