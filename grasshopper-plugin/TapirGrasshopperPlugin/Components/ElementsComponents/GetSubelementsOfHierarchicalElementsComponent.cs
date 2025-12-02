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
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject inputElements))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out string subType))
            {
                return;
            }

            if (!TryGetConvertedValues(
                    CommandName,
                    inputElements,
                    ToAddOn,
                    JHelp.Deserialize<SubelementsObj>,
                    out SubelementsObj response))
            {
                return;
            }

            var hierarchicalElements = new List<ElementGuidWrapper>();
            var subelementsOfHierarchicals = new DataTree<ElementGuidWrapper>();

            for (var i = 0; i < response.Subelements.Count; i++)
            {
                List<ElementGuidWrapper> subelements = null;

                if (subType == "CurtainWallSegment")
                {
                    subelements = response.Subelements[i].CurtainWallSegments;
                }
                else if (subType == "CurtainWallFrame")
                {
                    subelements = response.Subelements[i].CurtainWallFrames;
                }
                else if (subType == "CurtainWallPanel")
                {
                    subelements = response.Subelements[i].CurtainWallPanels;
                }
                else if (subType == "CurtainWallJunction")
                {
                    subelements = response.Subelements[i].CurtainWallJunctions;
                }
                else if (subType == "CurtainWallAccessory")
                {
                    subelements = response.Subelements[i]
                        .CurtainWallAccessories;
                }
                else if (subType == "StairRiser")
                {
                    subelements = response.Subelements[i].StairRisers;
                }
                else if (subType == "StairTread")
                {
                    subelements = response.Subelements[i].StairTreads;
                }
                else if (subType == "StairStructure")
                {
                    subelements = response.Subelements[i].StairStructures;
                }
                else if (subType == "RailingNode")
                {
                    subelements = response.Subelements[i].RailingNodes;
                }
                else if (subType == "RailingSegment")
                {
                    subelements = response.Subelements[i].RailingSegments;
                }
                else if (subType == "RailingPost")
                {
                    subelements = response.Subelements[i].RailingPosts;
                }
                else if (subType == "RailingRailEnd")
                {
                    subelements = response.Subelements[i].RailingRailEnds;
                }
                else if (subType == "RailingRailConnection")
                {
                    subelements = response.Subelements[i]
                        .RailingRailConnections;
                }
                else if (subType == "RailingHandrailEnd")
                {
                    subelements = response.Subelements[i].RailingHandrailEnds;
                }
                else if (subType == "RailingHandrailConnection")
                {
                    subelements = response.Subelements[i]
                        .RailingHandrailConnections;
                }
                else if (subType == "RailingToprailEnd")
                {
                    subelements = response.Subelements[i].RailingToprailEnds;
                }
                else if (subType == "RailingToprailConnection")
                {
                    subelements = response.Subelements[i]
                        .RailingToprailConnections;
                }
                else if (subType == "RailingRail")
                {
                    subelements = response.Subelements[i].RailingRails;
                }
                else if (subType == "RailingToprail")
                {
                    subelements = response.Subelements[i].RailingToprails;
                }
                else if (subType == "RailingHandrail")
                {
                    subelements = response.Subelements[i].RailingHandrails;
                }
                else if (subType == "RailingPattern")
                {
                    subelements = response.Subelements[i].RailingPatterns;
                }
                else if (subType == "RailingInnerPost")
                {
                    subelements = response.Subelements[i].RailingInnerPosts;
                }
                else if (subType == "RailingPanel")
                {
                    subelements = response.Subelements[i].RailingPanels;
                }
                else if (subType == "RailingBalusterSet")
                {
                    subelements = response.Subelements[i].RailingBalusterSets;
                }
                else if (subType == "RailingBaluster")
                {
                    subelements = response.Subelements[i].RailingBalusters;
                }
                else if (subType == "BeamSegment")
                {
                    subelements = response.Subelements[i].BeamSegments;
                }
                else if (subType == "ColumnSegment")
                {
                    subelements = response.Subelements[i].ColumnSegments;
                }

                if (subelements == null || subelements.Count == 0)
                {
                    continue;
                }

                hierarchicalElements.Add(
                    new ElementGuidWrapper()
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