using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class SetLayerCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateLayerCombinations";

        public SetLayerCombinationsComponent()
            : base(
                "SetLayerCombinations",
                "Set the details of layer combinations.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "List of layer combination attribute Guids.");

            InTexts(
                "Name",
                "List of the names of the layer combinations.");

            InGenericTree(
                "LayerAttributeGuids",
                "Tree of identifiers of the layers to be included in the layer combinations.");

            InBooleanTree(
                "IsHiddenLayers",
                "Tree of visibility of the layers in the layer combinations.");

            InBooleanTree(
                "IsLockedLayers",
                "Tree of lock states of the layers in the layer combinations.");


            InBooleanTree(
                "IsWireframeLayers",
                "Tree of wireframe modes of the layers in the layer combinations.");

            InIntegerTree(
                "IntersectionGroupsOfLayers",
                "Tree of intersection groups of the layers in the layer combinations.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!AttributesObj.TryCreate(
                    da,
                    0,
                    out AttributesObj input))
            {
                return;
            }

            if (!da.TryGetItems(
                    1,
                    out List<string> names))
            {
                return;
            }

            if (input.Attributes.Count != names.Count)
            {
                this.AddError("Attribute to Name count mismatch!");
                return;
            }

            if (!da.TryGetTree(
                    2,
                    out GH_Structure<IGH_Goo> layerAttributeGuidsInput))
            {
                return;
            }

            if (!da.TryGetTree(
                    3,
                    out GH_Structure<GH_Boolean> isHiddenLayers))
            {
                return;
            }

            if (!da.TryGetTree(
                    4,
                    out GH_Structure<GH_Boolean> isLockedLayers))
            {
                return;
            }

            if (!da.TryGetTree(
                    5,
                    out GH_Structure<GH_Boolean> isWireframeLayers))
            {
                return;
            }

            if (!da.TryGetTree(
                    6,
                    out GH_Structure<GH_Integer> intersectionGroupsOfLayers))
            {
                return;
            }

            if
                (input.Attributes.Count <
                 layerAttributeGuidsInput.Branches.Count ||
                 (layerAttributeGuidsInput.Branches.Count !=
                  isHiddenLayers.Branches.Count &&
                  (isHiddenLayers.Branches.Count != 1 ||
                   isHiddenLayers.Branches[0].Count != 1)) ||
                 (layerAttributeGuidsInput.Branches.Count !=
                  isLockedLayers.Branches.Count &&
                  (isLockedLayers.Branches.Count != 1 ||
                   isLockedLayers.Branches[0].Count != 1)) ||
                 (layerAttributeGuidsInput.Branches.Count !=
                  isWireframeLayers.Branches.Count &&
                  (isWireframeLayers.Branches.Count != 1 ||
                   isWireframeLayers.Branches[0].Count != 1)) ||
                 (layerAttributeGuidsInput.Branches.Count !=
                  intersectionGroupsOfLayers.Branches.Count &&
                  (intersectionGroupsOfLayers.Branches.Count != 1 ||
                   intersectionGroupsOfLayers.Branches[0].Count != 1)))
            {
                this.AddError("Tree branch count inequality!");
                return;
            }

            var layerCombinationDataArray = new LayerCombinationDataArrayObj()
            {
                LayerCombinationDataArray =
                    new List<LayerCombinationDataObj>(),
                OverwriteExisting = true
            };

            var index = 0;

            foreach (var attributeId in input.Attributes)
            {
                var layerCombinationData = new LayerCombinationDataObj();
                layerCombinationData.AttributeId = attributeId.AttributeId;
                layerCombinationData.Name = names[index];
                layerCombinationData.Layers = new List<ContainedLayerObj>();
                var path = new GH_Path(index);
                var layerGuidsGooList =
                    layerAttributeGuidsInput.Branches[
                        layerAttributeGuidsInput.Branches.Count == 1
                            ? 0
                            : index];
                var isHiddenGooList =
                    isHiddenLayers.Branches[isHiddenLayers.Branches.Count == 1
                        ? 0
                        : index];
                var isLockedGooList =
                    isLockedLayers.Branches[isLockedLayers.Branches.Count == 1
                        ? 0
                        : index];
                var isWireframeGooList =
                    isWireframeLayers.Branches[
                        isWireframeLayers.Branches.Count == 1 ? 0 : index];
                var intersectionGroupGooList =
                    intersectionGroupsOfLayers.Branches[
                        intersectionGroupsOfLayers.Branches.Count == 1
                            ? 0
                            : index];

                var layerCount = layerGuidsGooList.Count;

                for (var i = 0; i < layerCount; i++)
                {
                    var containedLayer = new ContainedLayerObj()
                    {
                        AttributeId =
                            AttributeIdObj.FromString(
                                (layerGuidsGooList.Count == 1
                                    ? layerGuidsGooList[0]
                                    : layerGuidsGooList[i]).ToString()),
                        IsHidden =
                            ((isHiddenGooList.Count == 1
                                ? isHiddenGooList[0]
                                : isHiddenGooList[i]) as GH_Boolean).Value,
                        IsLocked =
                            ((isLockedGooList.Count == 1
                                ? isLockedGooList[0]
                                : isLockedGooList[i]) as GH_Boolean).Value,
                        IsWireframe =
                            ((isWireframeGooList.Count == 1
                                ? isWireframeGooList[0]
                                : isWireframeGooList[i]) as GH_Boolean)
                            .Value,
                        IntersectionGroupNr =
                            ((intersectionGroupGooList.Count == 1
                                    ? intersectionGroupGooList[0]
                                    : intersectionGroupGooList[i]) as
                                GH_Integer).Value,
                    };
                    layerCombinationData.Layers.Add(containedLayer);
                }

                layerCombinationDataArray.LayerCombinationDataArray.Add(
                    layerCombinationData);
                index++;
            }

            SetArchiCadValues(
                CommandName,
                layerCombinationDataArray);
        }

        public override Guid ComponentGuid =>
            new Guid("4993bc97-b15f-4217-972c-b88acef3cc62");
    }
}