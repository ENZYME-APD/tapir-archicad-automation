using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Policy;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;

namespace TapirGrasshopperPlugin.Data
{
    public class ElementIdObj
    {
        public override string ToString ()
        {
            return Guid;
        }

        [JsonProperty ("guid")]
        public string Guid;
    }

    public class ElementIdItemObj
    {
        public override string ToString ()
        {
            return ElementId.ToString ();
        }

        [JsonProperty ("elementId")]
        public ElementIdObj ElementId;
    }

    public class ElementsObj
    {
        [JsonProperty ("elements")]
        public List<ElementIdItemObj> Elements;
    }

    public class HierarchicalElementsObj
    {
        [JsonProperty ("hierarchicalElements")]
        public List<ElementIdItemObj> Elements;
    }

    public class ElementPropertyValueObj
    {
        [JsonProperty ("elementId")]
        public ElementIdObj ElementId;

        [JsonProperty ("propertyId")]
        public PropertyIdObj PropertyId;

        [JsonProperty ("propertyValue")]
        public PropertyValueObj PropertyValue;
    }

    public class ElementPropertyValuesObj
    {
        [JsonProperty ("elementPropertyValues")]
        public List<ElementPropertyValueObj> ElementPropertyValues;
    }

    public class DetailsOfElementObj
    {
        [JsonProperty ("type")]
        public string Type;

        [JsonProperty ("floorIndex")]
        public int FloorIndex;

        [JsonProperty ("layerIndex")]
        public int LayerIndex;

        [JsonProperty ("drawIndex")]
        public int DrawIndex;

        [JsonProperty ("details")]
        public JObject Details;
    }

    public class DetailsOfElementsObj
    {
        [JsonProperty ("detailsOfElements")]
        public List<DetailsOfElementObj> DetailsOfElements;
    }

    public class SubelementsObj
    {
        [JsonProperty ("cWallSegments")]
        public List<ElementIdItemObj> CurtainWallSegments;

        [JsonProperty ("cWallFrames")]
        public List<ElementIdItemObj> CurtainWallFrames;

        [JsonProperty ("cWallPanels")]
        public List<ElementIdItemObj> CurtainWallPanels;

        [JsonProperty ("cWallJunctions")]
        public List<ElementIdItemObj> CurtainWallJunctions;

        [JsonProperty ("cWallAccessories")]
        public List<ElementIdItemObj> CurtainWallAccessories;

        [JsonProperty ("stairRisers")]
        public List<ElementIdItemObj> StairRisers;

        [JsonProperty ("stairTreads")]
        public List<ElementIdItemObj> StairTreads;

        [JsonProperty ("stairStructures")]
        public List<ElementIdItemObj> StairStructures;

        [JsonProperty ("railingNodes")]
        public List<ElementIdItemObj> RailingNodes;

        [JsonProperty ("railingSegments")]
        public List<ElementIdItemObj> RailingSegments;

        [JsonProperty ("railingPosts")]
        public List<ElementIdItemObj> RailingPosts;

        [JsonProperty ("railingRailEnds")]
        public List<ElementIdItemObj> RailingRailEnds;

        [JsonProperty ("railingRailConnections")]
        public List<ElementIdItemObj> RailingRailConnections;

        [JsonProperty ("railingHandrailEnds")]
        public List<ElementIdItemObj> RailingHandrailEnds;

        [JsonProperty ("railingHandrailConnections")]
        public List<ElementIdItemObj> RailingHandrailConnections;

        [JsonProperty ("railingToprailEnds")]
        public List<ElementIdItemObj> RailingToprailEnds;

        [JsonProperty ("railingToprailConnections")]
        public List<ElementIdItemObj> RailingToprailConnections;

        [JsonProperty ("railingRails")]
        public List<ElementIdItemObj> RailingRails;

        [JsonProperty ("railingToprails")]
        public List<ElementIdItemObj> RailingToprails;

        [JsonProperty ("railingHandrails")]
        public List<ElementIdItemObj> RailingHandrails;

        [JsonProperty ("railingPatterns")]
        public List<ElementIdItemObj> RailingPatterns;

        [JsonProperty ("railingInnerPosts")]
        public List<ElementIdItemObj> RailingInnerPosts;

        [JsonProperty ("railingPanels")]
        public List<ElementIdItemObj> RailingPanels;

        [JsonProperty ("railingBalusterSets")]
        public List<ElementIdItemObj> RailingBalusterSets;

        [JsonProperty ("railingBalusters")]
        public List<ElementIdItemObj> RailingBalusters;

        [JsonProperty ("beamSegments")]
        public List<ElementIdItemObj> BeamSegments;

        [JsonProperty ("columnSegments")]
        public List<ElementIdItemObj> ColumnSegments;
    }

    public class SubelementsOfHierarchicalElementsObj
    {
        [JsonProperty ("subelementsOfHierarchicalElements")]
        public List<SubelementsObj> SubelementsOfHierarchicalElements;
    }

    public class GDLParameterDetailsObj
    {
        [JsonProperty ("name")]
        public string Name;

        [JsonProperty ("index")]
        public string Index;

        [JsonProperty ("type")]
        public string Type;

        [JsonProperty ("dimension1")]
        public int Dimension1;

        [JsonProperty ("dimension2")]
        public int Dimension2;

        [JsonProperty ("value")]
        public Object Value;
    }

    public class GDLParameterListObj
    {
        public override string ToString ()
        {
            List<string> paramNames = new List<string> ();
            foreach (GDLParameterDetailsObj param in Parameters) {
                paramNames.Add (param.Name);
            }
            return String.Join (", ", paramNames);
        }

        [JsonProperty ("parameters")]
        public List<GDLParameterDetailsObj> Parameters;
    }

    public class Box3DObj
    {
        [JsonProperty ("xMin")]
        public double XMin;

        [JsonProperty ("yMin")]
        public double YMin;

        [JsonProperty ("zMin")]
        public double ZMin;

        [JsonProperty ("xMax")]
        public double XMax;

        [JsonProperty ("yMax")]
        public double YMax;

        [JsonProperty ("zMax")]
        public double ZMax;
    }

    public class BoundingBox3DObj
    {
        [JsonProperty ("boundingBox3D")]
        public Box3DObj BoundingBox3D;
    }

    public class BoundingBoxes3DObj
    {
        [JsonProperty ("boundingBoxes3D")]
        public List<BoundingBox3DObj> BoundingBoxes3D;
    }
}
