using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetLayerCombinationsComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Get the details of layer combinations.";
        public override string CommandName => "GetLayerCombinations";

        public GetLayerCombinationsComponent()
            : base(
                "Layer combinations",
                "Layer combinations",
                Doc,
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "List of layer combination attribute Guids.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Name",
                "List of the names of the layer combinations.");

            OutGenericTree(
                "LayerAttributeGuids",
                "Tree of identifiers of the layers included in the layer combinations.");

            OutBooleanTree(
                "IsHiddenLayers",
                "Tree of visibility of the layers in the layer combinations.");

            OutBooleanTree(
                "IsLockedLayers",
                "Tree of lock states of the layers in the layer combinations.");

            OutBooleanTree(
                "IsWireframeLayers",
                "Tree of wireframe modes of the layers in the layer combinations.");

            OutIntegerTree(
                "IntersectionGroupsOfLayers",
                "Tree of intersection groups of the layers in the layer combinations.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var attributes = AttributesObj.Create(
                da,
                0);
            if (attributes == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input AttributeGuids failed to collect data.");
                return;
            }

            if (!GetConvertedResponse(
                    CommandName,
                    attributes,
                    out LayerCombinationsObj layerCombinations)) { return; }

            var attributeNames = new List<string>();
            var layerAttributeIds = new DataTree<AttributeIdItemObj>();
            var isHiddenLayers = new DataTree<bool>();
            var isLockedLayers = new DataTree<bool>();
            var isWireframeLayers = new DataTree<bool>();
            var intersectionGroupsOfLayers = new DataTree<int>();

            for (var i = 0; i < layerCombinations.LayerCombinations.Count; i++)
            {
                var layerCombination = layerCombinations.LayerCombinations[i];
                attributeNames.Add(layerCombination.LayerCombination.Name);
                var layerIds = new List<AttributeIdItemObj>();
                var isHiddens = new List<bool>();
                var isLockeds = new List<bool>();
                var isWireframes = new List<bool>();
                var intersectionGroups = new List<int>();

                foreach (var layer in layerCombination.LayerCombination.Layers)
                {
                    layerIds.Add(
                        new AttributeIdItemObj()
                        {
                            AttributeId = layer.AttributeId
                        });
                    isHiddens.Add(layer.IsHidden);
                    isLockeds.Add(layer.IsLocked);
                    isWireframes.Add(layer.IsWireframe);
                    intersectionGroups.Add(layer.IntersectionGroupNr);
                }

                layerAttributeIds.AddRange(
                    layerIds,
                    new GH_Path(i));
                isHiddenLayers.AddRange(
                    isHiddens,
                    new GH_Path(i));
                isLockedLayers.AddRange(
                    isLockeds,
                    new GH_Path(i));
                isWireframeLayers.AddRange(
                    isWireframes,
                    new GH_Path(i));
                intersectionGroupsOfLayers.AddRange(
                    intersectionGroups,
                    new GH_Path(i));
            }

            da.SetDataList(
                0,
                attributeNames);
            da.SetDataTree(
                1,
                layerAttributeIds);
            da.SetDataTree(
                2,
                isHiddenLayers);
            da.SetDataTree(
                3,
                isLockedLayers);
            da.SetDataTree(
                4,
                isWireframeLayers);
            da.SetDataTree(
                5,
                intersectionGroupsOfLayers);
        }

        public override Guid ComponentGuid =>
            new Guid("a4cebe10-f489-4cd6-a174-45b158a33365");
    }
}