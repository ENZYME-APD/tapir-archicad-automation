using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Element
{
    // The rectangle of a marquee, as the GetMarquee response and the
    // SetMarquee parameters carry it (BoundingBox2D of the add-on).
    public class MarqueeBox
    {
        [JsonProperty("xMin")]
        public double XMin;

        [JsonProperty("yMin")]
        public double YMin;

        [JsonProperty("xMax")]
        public double XMax;

        [JsonProperty("yMax")]
        public double YMax;
    }

    // Response of the GetMarquee command. Only type is always present: the
    // other fields are absent when there is no marquee, box and rotationAngle
    // are only given for the rectangle shapes.
    public class MarqueeResponse
    {
        // Values of Type; suffixed so they do not clash with the fields below.
        public const string NoneType = "none";
        public const string RectangleType = "rectangle";
        public const string RotatedRectangleType = "rotatedRectangle";
        public const string PolygonType = "polygon";

        [JsonProperty("type")]
        public string Type;

        [JsonProperty("multiStory")]
        public bool? MultiStory;

        [JsonProperty("box")]
        public MarqueeBox Box;

        [JsonProperty("rotationAngle")]
        public double? RotationAngle;

        [JsonProperty("polygon")]
        public List<Point2D> Polygon;

        public bool HasMarquee => Type != null && Type != NoneType;
    }

    // Parameters of the SetMarquee command. Either polygon or box is sent,
    // neither of them removes the marquee; the schema forbids unknown
    // fields, so the unused ones are left out of the JSON.
    public class SetMarqueeParameters
    {
        [JsonProperty(
            "polygon",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<Point2D> Polygon;

        [JsonProperty(
            "box",
            NullValueHandling = NullValueHandling.Ignore)]
        public MarqueeBox Box;

        [JsonProperty(
            "multiStory",
            NullValueHandling = NullValueHandling.Ignore)]
        public bool? MultiStory;
    }
}
