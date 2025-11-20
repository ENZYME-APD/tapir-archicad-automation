using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class HierarchicalElementsObj
    {
        [JsonProperty("hierarchicalElements")]
        public List<ElementIdItemObj> Elements;
    }

    public class GetSubelementsOfHierarchicalElementsComponent
        : ArchicadAccessorComponent
    {
        public static string Doc =>
            "Gets the subelements of the given hierarchical elements.";

        public override string CommandName =>
            "GetSubelementsOfHierarchicalElements";

        public GetSubelementsOfHierarchicalElementsComponent()
            : base(
                "Subelements",
                "Subelems",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids of hierarchical elements to get subelements of.");

            InText(
                "SubelemType",
                "Type of subelements.");
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ElementGuids",
                "ElementGuids",
                "Elements Guids of the found hierarchical elements which has subelements with the given type.",
                GH_ParamAccess.list);
            pManager.AddGenericParameter(
                "SubelementGuids",
                "SubelementGuids",
                "Subelements with the given type.",
                GH_ParamAccess.tree);
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new ElementTypeValueList(ElementTypeValueListType.SubElementsOnly)
                .AddAsSource(
                    this,
                    1);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var inputElements = ElementsObj.Create(
                da,
                0);
            if (inputElements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            var subelemType = "";
            if (!da.GetData(
                    1,
                    ref subelemType))
            {
                return;
            }

            if (!GetConvertedResponse(
                    CommandName,
                    inputElements,
                    out SubelementsObj subElementsObj))
            {
                return;
            }

            var hierarchicalElements = new List<ElementIdItemObj>();
            var subelementsOfHierarchicals = new DataTree<ElementIdItemObj>();

            for (var i = 0; i < subElementsObj.Subelements.Count; i++)
            {
                List<ElementIdItemObj> subelements = null;
                if (subelemType == "CurtainWallSegment")
                {
                    subelements = subElementsObj.Subelements[i]
                        .CurtainWallSegments;
                }
                else if (subelemType == "CurtainWallFrame")
                {
                    subelements = subElementsObj.Subelements[i]
                        .CurtainWallFrames;
                }
                else if (subelemType == "CurtainWallPanel")
                {
                    subelements = subElementsObj.Subelements[i]
                        .CurtainWallPanels;
                }
                else if (subelemType == "CurtainWallJunction")
                {
                    subelements = subElementsObj.Subelements[i]
                        .CurtainWallJunctions;
                }
                else if (subelemType == "CurtainWallAccessory")
                {
                    subelements = subElementsObj.Subelements[i]
                        .CurtainWallAccessories;
                }
                else if (subelemType == "StairRiser")
                {
                    subelements = subElementsObj.Subelements[i].StairRisers;
                }
                else if (subelemType == "StairTread")
                {
                    subelements = subElementsObj.Subelements[i].StairTreads;
                }
                else if (subelemType == "StairStructure")
                {
                    subelements = subElementsObj.Subelements[i].StairStructures;
                }
                else if (subelemType == "RailingNode")
                {
                    subelements = subElementsObj.Subelements[i].RailingNodes;
                }
                else if (subelemType == "RailingSegment")
                {
                    subelements = subElementsObj.Subelements[i].RailingSegments;
                }
                else if (subelemType == "RailingPost")
                {
                    subelements = subElementsObj.Subelements[i].RailingPosts;
                }
                else if (subelemType == "RailingRailEnd")
                {
                    subelements = subElementsObj.Subelements[i].RailingRailEnds;
                }
                else if (subelemType == "RailingRailConnection")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingRailConnections;
                }
                else if (subelemType == "RailingHandrailEnd")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingHandrailEnds;
                }
                else if (subelemType == "RailingHandrailConnection")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingHandrailConnections;
                }
                else if (subelemType == "RailingToprailEnd")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingToprailEnds;
                }
                else if (subelemType == "RailingToprailConnection")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingToprailConnections;
                }
                else if (subelemType == "RailingRail")
                {
                    subelements = subElementsObj.Subelements[i].RailingRails;
                }
                else if (subelemType == "RailingToprail")
                {
                    subelements = subElementsObj.Subelements[i].RailingToprails;
                }
                else if (subelemType == "RailingHandrail")
                {
                    subelements =
                        subElementsObj.Subelements[i].RailingHandrails;
                }
                else if (subelemType == "RailingPattern")
                {
                    subelements = subElementsObj.Subelements[i].RailingPatterns;
                }
                else if (subelemType == "RailingInnerPost")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingInnerPosts;
                }
                else if (subelemType == "RailingPanel")
                {
                    subelements = subElementsObj.Subelements[i].RailingPanels;
                }
                else if (subelemType == "RailingBalusterSet")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingBalusterSets;
                }
                else if (subelemType == "RailingBaluster")
                {
                    subelements =
                        subElementsObj.Subelements[i].RailingBalusters;
                }
                else if (subelemType == "BeamSegment")
                {
                    subelements = subElementsObj.Subelements[i].BeamSegments;
                }
                else if (subelemType == "ColumnSegment")
                {
                    subelements = subElementsObj.Subelements[i].ColumnSegments;
                }

                if (subelements == null || subelements.Count == 0)
                {
                    continue;
                }

                hierarchicalElements.Add(
                    new ElementIdItemObj()
                    {
                        ElementId = inputElements.Elements[i].ElementId
                    });
                subelementsOfHierarchicals.AddRange(
                    subelements,
                    new GH_Path(i));
            }

            da.SetDataList(
                0,
                hierarchicalElements);
            da.SetDataTree(
                1,
                subelementsOfHierarchicals);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Subelems;

        public override Guid ComponentGuid =>
            new Guid("c0e93009-a0b6-4d24-9963-ef6c2e2535ed");
    }
}