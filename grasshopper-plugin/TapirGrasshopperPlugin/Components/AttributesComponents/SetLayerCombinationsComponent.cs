using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Grasshopper;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.DocObjects;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class LayerCombinationDataObj
    {
        [JsonProperty ("attributeId")]
        public AttributeIdObj AttributeId;

        [JsonProperty ("name")]
        public string Name;

        [JsonProperty ("layers")]
        public List<ContainedLayerObj> Layers;
    }

    public class LayerCombinationDataArrayObj
    {
        [JsonProperty ("layerCombinationDataArray")]
        public List<LayerCombinationDataObj> LayerCombinationDataArray;

        [JsonProperty ("overwriteExisting")]
        public bool OverwriteExisting;
    }

    public class SetLayerCombinationsComponent : ArchicadExecutorComponent
    {
        public SetLayerCombinationsComponent ()
          : base (
                "Set Layer Combinations",
                "SetLayerCombinations",
                "Set the details of layer combinations.",
                "Attributes"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("AttributeGuids", "AttributeGuids", "List of layer combination attribute Guids.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Name", "Name", "List of the names of the layer combinations.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("LayerAttributeGuids", "LayerAttributeGuids", "Tree of identifiers of the layers to be included in the layer combinations.", GH_ParamAccess.tree);
            pManager.AddBooleanParameter ("IsHiddenLayers", "IsHiddenLayers", "Tree of visibility of the layers in the layer combinations.", GH_ParamAccess.tree);
            pManager.AddBooleanParameter ("IsLockedLayers", "IsLockedLayers", "Tree of lock states of the layers in the layer combinations.", GH_ParamAccess.tree);
            pManager.AddBooleanParameter ("IsWireframeLayers", "IsWireframeLayers", "Tree of wireframe modes of the layers in the layer combinations.", GH_ParamAccess.tree);
            pManager.AddIntegerParameter ("IntersectionGroupsOfLayers", "IntersectionGroupsOfLayers", "Tree of intersection groups of the layers in the layer combinations.", GH_ParamAccess.tree);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            AttributesObj attributes = AttributesObj.Create (DA, 0);
            if (attributes == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input AttributeGuids failed to collect data.");
                return;
            }

            List<string> names = new List<string> ();
            if (!DA.GetDataList (1, names)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input AttributeNames failed to collect data.");
                return;
            }

            if (attributes.Attributes.Count != names.Count) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "The number of AttributeGuids and Names must be the same.");
                return;
            }

            GH_Structure<IGH_Goo> layerAttributeGuidsInput;
            if (!DA.GetDataTree (2, out layerAttributeGuidsInput)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input LayerAttributeGuids failed to collect data.");
                return;
            }

            GH_Structure<GH_Boolean> isHiddenLayers;
            if (!DA.GetDataTree (3, out isHiddenLayers)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IsHiddenLayers failed to collect data.");
                return;
            }

            GH_Structure<GH_Boolean> isLockedLayers;
            if (!DA.GetDataTree (4, out isLockedLayers)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IsLockedLayers failed to collect data.");
                return;
            }

            GH_Structure<GH_Boolean> isWireframeLayers;
            if (!DA.GetDataTree (5, out isWireframeLayers)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IsWireframeLayers failed to collect data.");
                return;
            }

            GH_Structure<GH_Integer> intersectionGroupsOfLayers;
            if (!DA.GetDataTree (6, out intersectionGroupsOfLayers)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IntersectionGroupsOfLayers failed to collect data.");
                return;
            }

            if (attributes.Attributes.Count < layerAttributeGuidsInput.Branches.Count ||
                (layerAttributeGuidsInput.Branches.Count != isHiddenLayers.Branches.Count && (isHiddenLayers.Branches.Count != 1 || isHiddenLayers.Branches[0].Count != 1)) ||
                (layerAttributeGuidsInput.Branches.Count != isLockedLayers.Branches.Count && (isLockedLayers.Branches.Count != 1 || isLockedLayers.Branches[0].Count != 1)) ||
                (layerAttributeGuidsInput.Branches.Count != isWireframeLayers.Branches.Count && (isWireframeLayers.Branches.Count != 1 || isWireframeLayers.Branches[0].Count != 1)) ||
                (layerAttributeGuidsInput.Branches.Count != intersectionGroupsOfLayers.Branches.Count && (intersectionGroupsOfLayers.Branches.Count != 1 || intersectionGroupsOfLayers.Branches[0].Count != 1))) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "The number of paths in the input trees must match the number of input layers.");
                return;
            }

            LayerCombinationDataArrayObj layerCombinationDataArray = new LayerCombinationDataArrayObj () {
                LayerCombinationDataArray = new List<LayerCombinationDataObj> (),
                OverwriteExisting = true
            };

            int index = 0;
            foreach (AttributeIdItemObj attributeId in attributes.Attributes) {
                LayerCombinationDataObj layerCombinationData = new LayerCombinationDataObj ();
                layerCombinationData.AttributeId = attributeId.AttributeId;
                layerCombinationData.Name = names[index];
                layerCombinationData.Layers = new List<ContainedLayerObj> ();
                GH_Path path = new GH_Path (index);
                var layerGuidsGooList = layerAttributeGuidsInput.Branches[layerAttributeGuidsInput.Branches.Count == 1 ? 0 : index];
                var isHiddenGooList = isHiddenLayers.Branches[isHiddenLayers.Branches.Count == 1 ? 0 : index];
                var isLockedGooList = isLockedLayers.Branches[isLockedLayers.Branches.Count == 1 ? 0 : index];
                var isWireframeGooList = isWireframeLayers.Branches[isWireframeLayers.Branches.Count == 1 ? 0 : index];
                var intersectionGroupGooList = intersectionGroupsOfLayers.Branches[intersectionGroupsOfLayers.Branches.Count == 1 ? 0 : index];
                int layerCount = layerGuidsGooList.Count;
                for (int i = 0; i < layerCount; i++) {
                    ContainedLayerObj containedLayer = new ContainedLayerObj () {
                        AttributeId = AttributeIdObj.CreateFromGuidString ((layerGuidsGooList.Count == 1 ? layerGuidsGooList[0] : layerGuidsGooList[i]).ToString ()),
                        IsHidden = ((isHiddenGooList.Count == 1 ? isHiddenGooList[0] : isHiddenGooList[i]) as GH_Boolean).Value,
                        IsLocked = ((isLockedGooList.Count == 1 ? isLockedGooList[0] : isLockedGooList[i]) as GH_Boolean).Value,
                        IsWireframe = ((isWireframeGooList.Count == 1 ? isWireframeGooList[0] : isWireframeGooList[i]) as GH_Boolean).Value,
                        IntersectionGroupNr = ((intersectionGroupGooList.Count == 1 ? intersectionGroupGooList[0] : intersectionGroupGooList[i]) as GH_Integer).Value,
                    };
                    layerCombinationData.Layers.Add (containedLayer);
                }
                layerCombinationDataArray.LayerCombinationDataArray.Add (layerCombinationData);
                index++;
            }

            JObject layerCombinationDataArrayObj = JObject.FromObject (layerCombinationDataArray);
            CommandResponse response = SendArchicadAddOnCommand ("CreateLayerCombinations", layerCombinationDataArrayObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.SetLayerCombinations;

        public override Guid ComponentGuid => new Guid ("4993bc97-b15f-4217-972c-b88acef3cc62");
    }
}