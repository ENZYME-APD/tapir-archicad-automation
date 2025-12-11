using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

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
                "Names",
                "List of the names of the layer combinations.");

            InGenericTree(
                "LayerAttributeGuids",
                "Identifiers of the layers to be included in the combinations.");

            InBooleanTree(
                "IsHiddenLayers",
                "Visibility states of the layers in the combinations.");

            InBooleanTree(
                "IsLockedLayers",
                "Lock states of the layers in the combinations.");


            InBooleanTree(
                "IsWireframeLayers",
                "Wireframe states of the layers in the combinations.");

            InIntegerTree(
                "IntersectionGroups",
                "Intersection groups of the layers in the combinations.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out AttributesObject input))
            {
                return;
            }

            if (!da.TryGetList(
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

            var layerCombinationDataArray = new LayerCombinationDataArrayObject
            {
                LayerCombinationDataArray =
                    new List<LayerCombinationDataObject>(),
                OverwriteExisting = true
            };

            var index = 0;

            foreach (var attributeId in input.Attributes)
            {
                var layerCombinationData = new LayerCombinationDataObject();
                layerCombinationData.AttributeId = attributeId.AttributeId;
                layerCombinationData.Name = names[index];
                layerCombinationData.Layers = new List<ContainedLayerObject>();
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
                    var containedLayer = new ContainedLayerObject
                    {
                        AttributeId =
                            AttributeGuidObject.FromString(
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

            SetCadValues(
                CommandName,
                layerCombinationDataArray,
                ToAddOn);
        }

        public override Guid ComponentGuid =>
            new Guid("4993bc97-b15f-4217-972c-b88acef3cc62");
    }
}