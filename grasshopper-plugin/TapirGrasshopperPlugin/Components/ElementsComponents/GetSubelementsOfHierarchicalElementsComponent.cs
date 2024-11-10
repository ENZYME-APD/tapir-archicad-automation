using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
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

        public override void AddedToDocument (GH_Document document)
        {
            base.AddedToDocument (document);

            if (Params.Input[1].SourceCount == 0) {
                ElementTypeValueList.AddAsSource (this, 1, ElementTypeValueListType.SubElementsOnly);
            }
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            ElementsObj inputElements = ElementsObj.Create (DA, 0);
            if (inputElements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementIds failed to collect data.");
                return;
            }

            string subelemType = "";
            if (!DA.GetData (1, ref subelemType)) {
                return;
            }

            JObject inputElementsObj = JObject.FromObject (inputElements);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetSubelementsOfHierarchicalElements", inputElementsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            List<ElementIdItemObj> hierarchicalElements = new List<ElementIdItemObj> ();
            DataTree<ElementIdItemObj> subelementsOfHierarchicals = new DataTree<ElementIdItemObj> ();
            SubelementsObj subElementsObj = response.Result.ToObject<SubelementsObj> ();
            for (int i = 0; i < subElementsObj.Subelements.Count; i++) {
                List<ElementIdItemObj> subelements = null;
                if (subelemType == "CurtainWallSegment") {
                    subelements = subElementsObj.Subelements[i].CurtainWallSegments;
                } else if (subelemType == "CurtainWallFrame") {
                    subelements = subElementsObj.Subelements[i].CurtainWallFrames;
                } else if (subelemType == "CurtainWallPanel") {
                    subelements = subElementsObj.Subelements[i].CurtainWallPanels;
                } else if (subelemType == "CurtainWallJunction") {
                    subelements = subElementsObj.Subelements[i].CurtainWallJunctions;
                } else if (subelemType == "CurtainWallAccessory") {
                    subelements = subElementsObj.Subelements[i].CurtainWallAccessories;
                } else if (subelemType == "StairRiser") {
                    subelements = subElementsObj.Subelements[i].StairRisers;
                } else if (subelemType == "StairTread") {
                    subelements = subElementsObj.Subelements[i].StairTreads;
                } else if (subelemType == "StairStructure") {
                    subelements = subElementsObj.Subelements[i].StairStructures;
                } else if (subelemType == "RailingNode") {
                    subelements = subElementsObj.Subelements[i].RailingNodes;
                } else if (subelemType == "RailingSegment") {
                    subelements = subElementsObj.Subelements[i].RailingSegments;
                } else if (subelemType == "RailingPost") {
                    subelements = subElementsObj.Subelements[i].RailingPosts;
                } else if (subelemType == "RailingRailEnd") {
                    subelements = subElementsObj.Subelements[i].RailingRailEnds;
                } else if (subelemType == "RailingRailConnection") {
                    subelements = subElementsObj.Subelements[i].RailingRailConnections;
                } else if (subelemType == "RailingHandrailEnd") {
                    subelements = subElementsObj.Subelements[i].RailingHandrailEnds;
                } else if (subelemType == "RailingHandrailConnection") {
                    subelements = subElementsObj.Subelements[i].RailingHandrailConnections;
                } else if (subelemType == "RailingToprailEnd") {
                    subelements = subElementsObj.Subelements[i].RailingToprailEnds;
                } else if (subelemType == "RailingToprailConnection") {
                    subelements = subElementsObj.Subelements[i].RailingToprailConnections;
                } else if (subelemType == "RailingRail") {
                    subelements = subElementsObj.Subelements[i].RailingRails;
                } else if (subelemType == "RailingToprail") {
                    subelements = subElementsObj.Subelements[i].RailingToprails;
                } else if (subelemType == "RailingHandrail") {
                    subelements = subElementsObj.Subelements[i].RailingHandrails;
                } else if (subelemType == "RailingPattern") {
                    subelements = subElementsObj.Subelements[i].RailingPatterns;
                } else if (subelemType == "RailingInnerPost") {
                    subelements = subElementsObj.Subelements[i].RailingInnerPosts;
                } else if (subelemType == "RailingPanel") {
                    subelements = subElementsObj.Subelements[i].RailingPanels;
                } else if (subelemType == "RailingBalusterSet") {
                    subelements = subElementsObj.Subelements[i].RailingBalusterSets;
                } else if (subelemType == "RailingBaluster") {
                    subelements = subElementsObj.Subelements[i].RailingBalusters;
                } else if (subelemType == "BeamSegment") {
                    subelements = subElementsObj.Subelements[i].BeamSegments;
                } else if (subelemType == "ColumnSegment") {
                    subelements = subElementsObj.Subelements[i].ColumnSegments;
                }
                if (subelements == null || subelements.Count == 0) {
                    continue;
                }
                hierarchicalElements.Add (new ElementIdItemObj () {
                    ElementId = inputElements.Elements[i].ElementId
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