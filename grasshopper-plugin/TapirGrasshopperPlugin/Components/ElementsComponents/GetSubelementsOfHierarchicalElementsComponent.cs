using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetSubelementsOfHierarchicalElementsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetSubelementsOfHierarchicalElements";

        public GetSubelementsOfHierarchicalElementsComponent()
            : base(
                "Subelements",
                "Gets the subelements of the given hierarchical elements.",
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

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Elements Guids of the found hierarchical elements which has subelements with the given type.");

            OutGenericTree(
                "SubelementGuids",
                "Subelements with the given type.");
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
            if (!ElementsObj.TryCreate(
                    da,
                    0,
                    out ElementsObj inputElements))
            {
                return;
            }

            if (!da.TryGetItem(
                    1,
                    out string subType))
            {
                return;
            }

            if (!TryGetConvertedResponse(
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

                if (subType == "CurtainWallSegment")
                {
                    subelements = subElementsObj.Subelements[i]
                        .CurtainWallSegments;
                }
                else if (subType == "CurtainWallFrame")
                {
                    subelements = subElementsObj.Subelements[i]
                        .CurtainWallFrames;
                }
                else if (subType == "CurtainWallPanel")
                {
                    subelements = subElementsObj.Subelements[i]
                        .CurtainWallPanels;
                }
                else if (subType == "CurtainWallJunction")
                {
                    subelements = subElementsObj.Subelements[i]
                        .CurtainWallJunctions;
                }
                else if (subType == "CurtainWallAccessory")
                {
                    subelements = subElementsObj.Subelements[i]
                        .CurtainWallAccessories;
                }
                else if (subType == "StairRiser")
                {
                    subelements = subElementsObj.Subelements[i].StairRisers;
                }
                else if (subType == "StairTread")
                {
                    subelements = subElementsObj.Subelements[i].StairTreads;
                }
                else if (subType == "StairStructure")
                {
                    subelements = subElementsObj.Subelements[i].StairStructures;
                }
                else if (subType == "RailingNode")
                {
                    subelements = subElementsObj.Subelements[i].RailingNodes;
                }
                else if (subType == "RailingSegment")
                {
                    subelements = subElementsObj.Subelements[i].RailingSegments;
                }
                else if (subType == "RailingPost")
                {
                    subelements = subElementsObj.Subelements[i].RailingPosts;
                }
                else if (subType == "RailingRailEnd")
                {
                    subelements = subElementsObj.Subelements[i].RailingRailEnds;
                }
                else if (subType == "RailingRailConnection")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingRailConnections;
                }
                else if (subType == "RailingHandrailEnd")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingHandrailEnds;
                }
                else if (subType == "RailingHandrailConnection")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingHandrailConnections;
                }
                else if (subType == "RailingToprailEnd")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingToprailEnds;
                }
                else if (subType == "RailingToprailConnection")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingToprailConnections;
                }
                else if (subType == "RailingRail")
                {
                    subelements = subElementsObj.Subelements[i].RailingRails;
                }
                else if (subType == "RailingToprail")
                {
                    subelements = subElementsObj.Subelements[i].RailingToprails;
                }
                else if (subType == "RailingHandrail")
                {
                    subelements =
                        subElementsObj.Subelements[i].RailingHandrails;
                }
                else if (subType == "RailingPattern")
                {
                    subelements = subElementsObj.Subelements[i].RailingPatterns;
                }
                else if (subType == "RailingInnerPost")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingInnerPosts;
                }
                else if (subType == "RailingPanel")
                {
                    subelements = subElementsObj.Subelements[i].RailingPanels;
                }
                else if (subType == "RailingBalusterSet")
                {
                    subelements = subElementsObj.Subelements[i]
                        .RailingBalusterSets;
                }
                else if (subType == "RailingBaluster")
                {
                    subelements =
                        subElementsObj.Subelements[i].RailingBalusters;
                }
                else if (subType == "BeamSegment")
                {
                    subelements = subElementsObj.Subelements[i].BeamSegments;
                }
                else if (subType == "ColumnSegment")
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