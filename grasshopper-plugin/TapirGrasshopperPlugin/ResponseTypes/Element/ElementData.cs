using System.Collections.Generic;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.IdObjects;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class ElementGuid : GuidObject<ElementGuid>
    {
    }

    public class ElementGuidItemObject
        : GuidItemObject<ElementGuid, ElementGuidItemObject>
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

    public class ElementsObj
        : GuidItemsObject<ElementGuid, ElementGuidItemObject, ElementsObj>
    {
        [JsonProperty("elements")]
        public List<ElementGuidItemObject> Elements;

        [JsonIgnore]
        public override List<ElementGuidItemObject> GuidItems
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

    public class ElementsAndPropertyIdsObj
    {
        [JsonProperty("elements")]
        public List<ElementGuidItemObject> Elements;

        [JsonProperty("properties")]
        public List<PropertyGuidItemObject> PropertyIds;
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
            Elements = new List<ElementGuidItemObject>();
            HighlightedColors = new List<List<int>>();
        }

        [JsonProperty("elements")]
        public List<ElementGuidItemObject> Elements;

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
        public List<ElementGuidItemObject> CurtainWallSegments;

        [JsonProperty("cWallFrames")]
        public List<ElementGuidItemObject> CurtainWallFrames;

        [JsonProperty("cWallPanels")]
        public List<ElementGuidItemObject> CurtainWallPanels;

        [JsonProperty("cWallJunctions")]
        public List<ElementGuidItemObject> CurtainWallJunctions;

        [JsonProperty("cWallAccessories")]
        public List<ElementGuidItemObject> CurtainWallAccessories;

        [JsonProperty("stairRisers")]
        public List<ElementGuidItemObject> StairRisers;

        [JsonProperty("stairTreads")]
        public List<ElementGuidItemObject> StairTreads;

        [JsonProperty("stairStructures")]
        public List<ElementGuidItemObject> StairStructures;

        [JsonProperty("railingNodes")]
        public List<ElementGuidItemObject> RailingNodes;

        [JsonProperty("railingSegments")]
        public List<ElementGuidItemObject> RailingSegments;

        [JsonProperty("railingPosts")]
        public List<ElementGuidItemObject> RailingPosts;

        [JsonProperty("railingRailEnds")]
        public List<ElementGuidItemObject> RailingRailEnds;

        [JsonProperty("railingRailConnections")]
        public List<ElementGuidItemObject> RailingRailConnections;

        [JsonProperty("railingHandrailEnds")]
        public List<ElementGuidItemObject> RailingHandrailEnds;

        [JsonProperty("railingHandrailConnections")]
        public List<ElementGuidItemObject> RailingHandrailConnections;

        [JsonProperty("railingToprailEnds")]
        public List<ElementGuidItemObject> RailingToprailEnds;

        [JsonProperty("railingToprailConnections")]
        public List<ElementGuidItemObject> RailingToprailConnections;

        [JsonProperty("railingRails")]
        public List<ElementGuidItemObject> RailingRails;

        [JsonProperty("railingToprails")]
        public List<ElementGuidItemObject> RailingToprails;

        [JsonProperty("railingHandrails")]
        public List<ElementGuidItemObject> RailingHandrails;

        [JsonProperty("railingPatterns")]
        public List<ElementGuidItemObject> RailingPatterns;

        [JsonProperty("railingInnerPosts")]
        public List<ElementGuidItemObject> RailingInnerPosts;

        [JsonProperty("railingPanels")]
        public List<ElementGuidItemObject> RailingPanels;

        [JsonProperty("railingBalusterSets")]
        public List<ElementGuidItemObject> RailingBalusterSets;

        [JsonProperty("railingBalusters")]
        public List<ElementGuidItemObject> RailingBalusters;

        [JsonProperty("beamSegments")]
        public List<ElementGuidItemObject> BeamSegments;

        [JsonProperty("columnSegments")]
        public List<ElementGuidItemObject> ColumnSegments;
    }

    public class SubelementsObj
    {
        [JsonProperty("subelements")]
        public List<SubelementsItemObj> Subelements;
    }


    public class Box3DObj
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
        public Box3DObj BoundingBox3D;

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

    public class BoundingBoxes3DObj
    {
        [JsonProperty("boundingBoxes3D")]
        public List<BoundingBox3DObj> BoundingBoxes3D;
    }
}