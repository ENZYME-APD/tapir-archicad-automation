using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class HierarchicalElementsObj
    {
        [JsonProperty ("hierarchicalElements")]
        public List<ElementIdItemObj> Elements;
    }

    public class GetSubelementsOfHierarchicalElementsComponent : ArchicadAccessorComponent
    {
        public GetSubelementsOfHierarchicalElementsComponent ()
          : base (
                "Subelements",
                "Subelems",
                "Gets the subelements of the given hierarchical elements.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids of hierarchical elements to get subelements of.", GH_ParamAccess.list);
            pManager.AddTextParameter ("SubelemType", "SubelemType", "Type of subelements.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids of the found hierarchical elements which has subelements with the given type.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("SubelementIds", "SubelementIds", "Subelements with the given type.", GH_ParamAccess.tree);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            List<ElementIdItemObj> elements = new List<ElementIdItemObj> ();
            if (!DA.GetDataList (0, elements)) {
                return;
            }

            string subelemType = "";
            if (!DA.GetData (1, ref subelemType)) {
                return;
            }

            HierarchicalElementsObj inputHierarchicalElements = new HierarchicalElementsObj () {
                Elements = elements
            };

            JObject inputHierarchicalElementsObj = JObject.FromObject (inputHierarchicalElements);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetSubelementsOfHierarchicalElements", inputHierarchicalElementsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            List<ElementIdItemObj> hierarchicalElements = new List<ElementIdItemObj> ();
            DataTree<ElementIdItemObj> subelementsOfHierarchicals = new DataTree<ElementIdItemObj> ();
            SubelementsOfHierarchicalElementsObj subelementsOfHierarchicalElementsObj = response.Result.ToObject<SubelementsOfHierarchicalElementsObj> ();
            for (int i = 0; i < subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements.Count; i++) {
                List<ElementIdItemObj> subelements = null;
                if (subelemType == "CurtainWallSegment") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].CurtainWallSegments;
                } else if (subelemType == "CurtainWallFrame") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].CurtainWallFrames;
                } else if (subelemType == "CurtainWallPanel") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].CurtainWallPanels;
                } else if (subelemType == "CurtainWallJunction") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].CurtainWallJunctions;
                } else if (subelemType == "CurtainWallAccessory") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].CurtainWallAccessories;
                } else if (subelemType == "StairRiser") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].StairRisers;
                } else if (subelemType == "StairTread") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].StairTreads;
                } else if (subelemType == "StairStructure") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].StairStructures;
                } else if (subelemType == "RailingNode") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingNodes;
                } else if (subelemType == "RailingSegment") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingSegments;
                } else if (subelemType == "RailingPost") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingPosts;
                } else if (subelemType == "RailingRailEnd") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingRailEnds;
                } else if (subelemType == "RailingRailConnection") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingRailConnections;
                } else if (subelemType == "RailingHandrailEnd") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingHandrailEnds;
                } else if (subelemType == "RailingHandrailConnection") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingHandrailConnections;
                } else if (subelemType == "RailingToprailEnd") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingToprailEnds;
                } else if (subelemType == "RailingToprailConnection") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingToprailConnections;
                } else if (subelemType == "RailingRail") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingRails;
                } else if (subelemType == "RailingToprail") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingToprails;
                } else if (subelemType == "RailingHandrail") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingHandrails;
                } else if (subelemType == "RailingPattern") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingPatterns;
                } else if (subelemType == "RailingInnerPost") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingInnerPosts;
                } else if (subelemType == "RailingPanel") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingPanels;
                } else if (subelemType == "RailingBalusterSet") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingBalusterSets;
                } else if (subelemType == "RailingBaluster") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].RailingBalusters;
                } else if (subelemType == "BeamSegment") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].BeamSegments;
                } else if (subelemType == "ColumnSegment") {
                    subelements = subelementsOfHierarchicalElementsObj.SubelementsOfHierarchicalElements[i].ColumnSegments;
                }
                if (subelements == null || subelements.Count == 0) {
                    continue;
                }
                hierarchicalElements.Add (new ElementIdItemObj () {
                    ElementId = elements[i].ElementId
                });
                subelementsOfHierarchicals.AddRange (subelements, new GH_Path (i));
            }

            DA.SetDataList (0, hierarchicalElements);
            DA.SetDataTree (1, subelementsOfHierarchicals);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.Subelems;

        public override Guid ComponentGuid => new Guid ("c0e93009-a0b6-4d24-9963-ef6c2e2535ed");
    }
}