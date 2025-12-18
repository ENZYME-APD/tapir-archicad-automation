using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

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
                "SubelementType",
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

            var subTypeEnum = EnumHelp.ToEnum<SubElementType>(subType);

            if (!TryGetConvertedCadValues(
                    CommandName,
                    inputElements,
                    ToAddOn,
                    JHelp.Deserialize<SubelementsObject>,
                    out SubelementsObject response))
            {
                return;
            }

            var hierarchicalElements = new List<ElementGuidWrapper>();
            var subelementsOfHierarchicals = new DataTree<ElementGuidWrapper>();

            for (var i = 0; i < response.Subelements.Count; i++)
            {
                List<ElementGuidWrapper> subelements = null;

                if (TypeMap.TryGetValue(
                        subTypeEnum,
                        out var selector))
                {
                    subelements = selector(response.Subelements[i]);
                }

                if (subelements == null || subelements.Count == 0)
                {
                    continue;
                }

                hierarchicalElements.Add(
                    new ElementGuidWrapper
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

        public Dictionary<SubElementType,
            Func<SubelementsItemObj, List<ElementGuidWrapper>>> TypeMap
        {
            get;
        } =
            new
                Dictionary<SubElementType, Func<SubelementsItemObj,
                    List<ElementGuidWrapper>>>()
                {
                    {
                        SubElementType.CurtainWallSegment,
                        x => x.CurtainWallSegments
                    },
                    {
                        SubElementType.CurtainWallFrame,
                        x => x.CurtainWallFrames
                    },
                    {
                        SubElementType.CurtainWallPanel,
                        x => x.CurtainWallPanels
                    },
                    {
                        SubElementType.CurtainWallJunction,
                        x => x.CurtainWallJunctions
                    },
                    {
                        SubElementType.CurtainWallAccessory,
                        x => x.CurtainWallAccessories
                    },
                    { SubElementType.StairTread, x => x.StairTreads },
                    { SubElementType.StairRiser, x => x.StairRisers },
                    { SubElementType.StairStructure, x => x.StairStructures },
                    { SubElementType.RailingNode, x => x.RailingNodes },
                    { SubElementType.RailingSegment, x => x.RailingSegments },
                    { SubElementType.RailingPost, x => x.RailingPosts },
                    { SubElementType.RailingRailEnd, x => x.RailingRailEnds },
                    {
                        SubElementType.RailingRailConnection,
                        x => x.RailingRailConnections
                    },
                    {
                        SubElementType.RailingHandrailEnd,
                        x => x.RailingHandrailEnds
                    },
                    {
                        SubElementType.RailingHandrailConnection,
                        x => x.RailingHandrailConnections
                    },
                    {
                        SubElementType.RailingToprailEnd,
                        x => x.RailingToprailEnds
                    },
                    {
                        SubElementType.RailingToprailConnection,
                        x => x.RailingToprailConnections
                    },
                    { SubElementType.RailingRail, x => x.RailingRails },
                    { SubElementType.RailingToprail, x => x.RailingToprails },
                    { SubElementType.RailingHandrail, x => x.RailingHandrails },
                    { SubElementType.RailingPattern, x => x.RailingPatterns },
                    {
                        SubElementType.RailingInnerPost,
                        x => x.RailingInnerPosts
                    },
                    { SubElementType.RailingPanel, x => x.RailingPanels },
                    {
                        SubElementType.RailingBalusterSet,
                        x => x.RailingBalusterSets
                    },
                    { SubElementType.RailingBaluster, x => x.RailingBalusters },
                    { SubElementType.BeamSegment, x => x.BeamSegments },
                    { SubElementType.ColumnSegment, x => x.ColumnSegments },
                };

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Subelems;

        public override Guid ComponentGuid =>
            new Guid("c0e93009-a0b6-4d24-9963-ef6c2e2535ed");
    }
}