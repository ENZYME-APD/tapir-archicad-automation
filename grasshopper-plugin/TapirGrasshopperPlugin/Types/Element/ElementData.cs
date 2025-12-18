using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System.Collections.Generic;
using System.Data;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class ElementGuid : GuidObject<ElementGuid>
    {
    }

    public class ElementGuidWrapper
        : GuidWrapper<ElementGuid, ElementGuidWrapper>
    {
        [JsonProperty("elementId")]
        public ElementGuid ElementId;

        [JsonIgnore]
        public override ElementGuid Id
        {
            get => ElementId;
            set => ElementId = value;
        }
    }

    public class ElementsObject
        : GuidItemsObject<ElementGuid, ElementGuidWrapper, ElementsObject>
    {
        [JsonProperty("elements")]
        public List<ElementGuidWrapper> Elements;

        [JsonIgnore]
        public override List<ElementGuidWrapper> GuidWrappers
        {
            get => Elements;
            set => Elements = value;
        }
    }

    public class ElementPropertyValueObj
    {
        [JsonProperty("elementId")]
        public ElementGuid ElementId;

        [JsonProperty("propertyId")]
        public PropertyGuidObject PropertyId;

        [JsonProperty("propertyValue")]
        public PropertyValueObj PropertyValue;
    }

    public class ElementPropertyValuesObj
    {
        [JsonProperty("elementPropertyValues")]
        public List<ElementPropertyValueObj> ElementPropertyValues;
    }

    public class HighlightElementsObj
    {
        public HighlightElementsObj()
        {
            Elements = new List<ElementGuidWrapper>();
            HighlightedColors = new List<List<int>>();
        }

        [JsonProperty("elements")]
        public List<ElementGuidWrapper> Elements;

        [JsonProperty("highlightedColors")]
        public List<List<int>> HighlightedColors;

        [JsonProperty(
            "nonHighlightedColor",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<int> NonHighlightedColor;

        [JsonProperty("wireframe3D")]
        public bool Wireframe3D;
    }

    public class DetailsOfElementObj
    {
        [JsonProperty("type")]
        public string Type;

        [JsonProperty("id")]
        public string ElementID;

        [JsonProperty("floorIndex")]
        public int FloorIndex;

        [JsonProperty("layerIndex")]
        public int LayerIndex;

        [JsonProperty("drawIndex")]
        public int DrawIndex;

        [JsonProperty("details")]
        public JObject Details;
    }

    public class DetailsOfElementsObj
    {
        [JsonProperty("detailsOfElements")]
        public List<DetailsOfElementObj> DetailsOfElements;
    }

    public class SubelementsItemObj
    {
        [JsonProperty("cWallSegments")]
        public List<ElementGuidWrapper> CurtainWallSegments;

        [JsonProperty("cWallFrames")]
        public List<ElementGuidWrapper> CurtainWallFrames;

        [JsonProperty("cWallPanels")]
        public List<ElementGuidWrapper> CurtainWallPanels;

        [JsonProperty("cWallJunctions")]
        public List<ElementGuidWrapper> CurtainWallJunctions;

        [JsonProperty("cWallAccessories")]
        public List<ElementGuidWrapper> CurtainWallAccessories;

        [JsonProperty("stairRisers")]
        public List<ElementGuidWrapper> StairRisers;

        [JsonProperty("stairTreads")]
        public List<ElementGuidWrapper> StairTreads;

        [JsonProperty("stairStructures")]
        public List<ElementGuidWrapper> StairStructures;

        [JsonProperty("railingNodes")]
        public List<ElementGuidWrapper> RailingNodes;

        [JsonProperty("railingSegments")]
        public List<ElementGuidWrapper> RailingSegments;

        [JsonProperty("railingPosts")]
        public List<ElementGuidWrapper> RailingPosts;

        [JsonProperty("railingRailEnds")]
        public List<ElementGuidWrapper> RailingRailEnds;

        [JsonProperty("railingRailConnections")]
        public List<ElementGuidWrapper> RailingRailConnections;

        [JsonProperty("railingHandrailEnds")]
        public List<ElementGuidWrapper> RailingHandrailEnds;

        [JsonProperty("railingHandrailConnections")]
        public List<ElementGuidWrapper> RailingHandrailConnections;

        [JsonProperty("railingToprailEnds")]
        public List<ElementGuidWrapper> RailingToprailEnds;

        [JsonProperty("railingToprailConnections")]
        public List<ElementGuidWrapper> RailingToprailConnections;

        [JsonProperty("railingRails")]
        public List<ElementGuidWrapper> RailingRails;

        [JsonProperty("railingToprails")]
        public List<ElementGuidWrapper> RailingToprails;

        [JsonProperty("railingHandrails")]
        public List<ElementGuidWrapper> RailingHandrails;

        [JsonProperty("railingPatterns")]
        public List<ElementGuidWrapper> RailingPatterns;

        [JsonProperty("railingInnerPosts")]
        public List<ElementGuidWrapper> RailingInnerPosts;

        [JsonProperty("railingPanels")]
        public List<ElementGuidWrapper> RailingPanels;

        [JsonProperty("railingBalusterSets")]
        public List<ElementGuidWrapper> RailingBalusterSets;

        [JsonProperty("railingBalusters")]
        public List<ElementGuidWrapper> RailingBalusters;

        [JsonProperty("beamSegments")]
        public List<ElementGuidWrapper> BeamSegments;

        [JsonProperty("columnSegments")]
        public List<ElementGuidWrapper> ColumnSegments;
    }

    public class SubelementsObject
    {
        [JsonProperty("subelements")]
        public List<SubelementsItemObj> Subelements;
    }


    public class Box3DObject
    {
        [JsonProperty("xMin")]
        public double XMin;

        [JsonProperty("yMin")]
        public double YMin;

        [JsonProperty("zMin")]
        public double ZMin;

        [JsonProperty("xMax")]
        public double XMax;

        [JsonProperty("yMax")]
        public double YMax;

        [JsonProperty("zMax")]
        public double ZMax;
    }

    public class BoundingBox3DObj
    {
        [JsonProperty("boundingBox3D")]
        public Box3DObject BoundingBox3D;

        public BoundingBox ToRhino()
        {
            return new BoundingBox()
            {
                Min = new Point3d(
                    BoundingBox3D.XMin,
                    BoundingBox3D.YMin,
                    BoundingBox3D.ZMin),
                Max = new Point3d(
                    BoundingBox3D.XMax,
                    BoundingBox3D.YMax,
                    BoundingBox3D.ZMax)
            };
        }
    }

    public class BoundingBoxes3DObject
    {
        [JsonProperty("boundingBoxes3D")]
        public List<BoundingBox3DObj> BoundingBoxes3D;
    }
}