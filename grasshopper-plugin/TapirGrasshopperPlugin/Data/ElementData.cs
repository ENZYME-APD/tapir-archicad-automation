using System;
using System.Collections.Generic;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace TapirGrasshopperPlugin.Data
{
    public abstract class IdObj<T> where T : IdObj<T>, new()
    {
        public static T Create (IGH_DataAccess DA, int index)
        {
            GH_ObjectWrapper id = new GH_ObjectWrapper ();
            if (!DA.GetData (index, ref id)) {
                return null;
            }
            return Create (id);
        }

        public static T Create (GH_ObjectWrapper obj)
        {
            if (obj.Value is T) {
                return obj.Value as T;
            } else if (obj.Value is GH_String) {
                GH_String stringValue = obj.Value as GH_String;
                return CreateFromGuidString (stringValue.ToString ());
            } else if (obj.Value.GetType ().GetProperty ("Guid") != null) {
                return CreateFromGuidString (obj.Value.GetType ().GetProperty ("Guid").ToString ());
            } else {
                return null;
            }
        }

        public static T CreateFromGuidString (string guidString)
        {
            if (System.Guid.TryParse (guidString, out _)) {
                return new T () {
                    Guid = guidString
                };
            } else {
                return null;
            }
        }

        public override string ToString ()
        {
            return Guid;
        }

        public bool IsNullGuid ()
        {
            return Guid == null || new System.Guid (Guid) == System.Guid.Empty;
        }

        [JsonProperty ("guid")]
        public string Guid;
    }

    public abstract class IdItemObj<I, T> where I : IdObj<I>, new() where T : IdItemObj<I, T>, new()
    {
        public static T Create (IGH_DataAccess DA, int index)
        {
            GH_ObjectWrapper idItem = new GH_ObjectWrapper ();
            if (!DA.GetData (index, ref idItem)) {
                return null;
            }
            return Create (idItem);
        }

        public static T Create (GH_ObjectWrapper obj)
        {
            if (obj.Value is T) {
                return obj.Value as T;
            } else {
                I id = IdObj<I>.Create (obj);
                if (id != null) {
                    return new T () {
                        Id = id
                    };
                } else {
                    return null;
                }
            }
        }

        public override string ToString ()
        {
            return Id.ToString ();
        }

        [JsonIgnore]
        public abstract I Id
        {
            get;
            set;
        }
    }

    public abstract class IdsObj<I, J, T> where I : IdObj<I>, new() where J : IdItemObj<I, J>, new() where T : IdsObj<I, J, T>, new()
    {
        public static T Create (IGH_DataAccess DA, int index)
        {
            List<GH_ObjectWrapper> ids = new List<GH_ObjectWrapper> ();
            if (!DA.GetDataList (index, ids)) {
                return null;
            }
            return Create (ids);
        }

        public static T Create (List<GH_ObjectWrapper> objects)
        {
            T ids = new T () {
                Ids = new List<J> ()
            };
            foreach (GH_ObjectWrapper obj in objects) {
                J id = IdItemObj<I, J>.Create (obj);
                if (id != null) {
                    ids.Ids.Add (id);
                } else {
                    return null;
                }
            }
            return ids;
        }

        [JsonIgnore]
        public abstract List<J> Ids
        {
            get;
            set;
        }
    }

    public class ElementIdObj : IdObj<ElementIdObj> { }

    public class ElementIdItemObj : IdItemObj<ElementIdObj, ElementIdItemObj>
    {
        [JsonProperty ("elementId")]
        public ElementIdObj ElementId;

        [JsonIgnore]
        public override ElementIdObj Id
        {
            get { return ElementId; }
            set { ElementId = value; }
        }
    }

    public class ElementsObj : IdsObj<ElementIdObj, ElementIdItemObj, ElementsObj>
    {
        [JsonProperty ("elements")]
        public List<ElementIdItemObj> Elements;

        [JsonIgnore]
        public override List<ElementIdItemObj> Ids
        {
            get { return Elements; }
            set { Elements = value; }
        }
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

    public class ElementsAndPropertyIdsObj
    {
        [JsonProperty ("elements")]
        public List<ElementIdItemObj> Elements;

        [JsonProperty ("properties")]
        public List<PropertyIdItemObj> PropertyIds;
    }

    public class ElementPropertyValuesObj
    {
        [JsonProperty ("elementPropertyValues")]
        public List<ElementPropertyValueObj> ElementPropertyValues;
    }

    public class HighlightElementsObj
    {
        public HighlightElementsObj ()
        {
            Elements = new List<ElementIdItemObj> ();
            HighlightedColors = new List<List<int>> ();
        }

        [JsonProperty ("elements")]
        public List<ElementIdItemObj> Elements;

        [JsonProperty ("highlightedColors")]
        public List<List<int>> HighlightedColors;

        [JsonProperty ("nonHighlightedColor", NullValueHandling = NullValueHandling.Ignore)]
        public List<int> NonHighlightedColor;

        [JsonProperty ("wireframe3D")]
        public bool Wireframe3D;
    }

    public class DetailsOfElementObj
    {
        [JsonProperty ("type")]
        public string Type;

        [JsonProperty ("id")]
        public string ElementID;

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

    public class SubelementsItemObj
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

    public class SubelementsObj
    {
        [JsonProperty ("subelements")]
        public List<SubelementsItemObj> Subelements;
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
