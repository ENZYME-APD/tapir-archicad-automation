using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
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
    public class ContainedLayerObj
    {
        [JsonProperty ("attributeId")]
        public AttributeIdObj AttributeId;

        [JsonProperty ("isHidden")]
        public bool IsHidden;

        [JsonProperty ("isLocked")]
        public bool IsLocked;

        [JsonProperty ("isWireframe")]
        public bool IsWireframe;

        [JsonProperty ("intersectionGroupNr")]
        public int IntersectionGroupNr;
    }

    public class LayerCombinationDetailsObj
    {
        [JsonProperty ("attributeId")]
        public AttributeIdObj AttributeId;

        [JsonProperty ("attributeIndex")]
        public int AttributeIndex;

        [JsonProperty ("name")]
        public string Name;

        [JsonProperty ("layers")]
        public List<ContainedLayerObj> Layers;
    }

    public class LayerCombinationObj
    {
        [JsonProperty ("layerCombination")]
        public LayerCombinationDetailsObj LayerCombination;
    }

    public class LayerCombinationsObj
    {
        [JsonProperty ("layerCombinations")]
        public List<LayerCombinationObj> LayerCombinations;
    }

    public class GetLayerCombinationsComponent : ArchicadAccessorComponent
    {
        public GetLayerCombinationsComponent ()
          : base (
                "Layer Combinations",
                "LayerCombinations",
                "Get the details of layer combinations.",
                "Attributes"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("AttributeGuids", "AttributeGuids", "List of layer combination attribute Guids.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter ("Name", "Name", "List of the names of the layer combinations.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("LayerAttributeGuids", "LayerAttributeGuids", "Tree of identifiers of the layers included in the layer combinations.", GH_ParamAccess.tree);
            pManager.AddBooleanParameter ("IsHiddenLayers", "IsHiddenLayers", "Tree of visibility of the layers in the layer combinations.", GH_ParamAccess.tree);
            pManager.AddBooleanParameter ("IsLockedLayers", "IsLockedLayers", "Tree of lock states of the layers in the layer combinations.", GH_ParamAccess.tree);
            pManager.AddBooleanParameter ("IsWireframeLayers", "IsWireframeLayers", "Tree of wireframe modes of the layers in the layer combinations.", GH_ParamAccess.tree);
            pManager.AddIntegerParameter ("IntersectionGroupsOfLayers", "IntersectionGroupsOfLayers", "Tree of intersection groups of the layers in the layer combinations.", GH_ParamAccess.tree);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            AttributesObj attributes = AttributesObj.Create (DA, 0);
            if (attributes == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input AttributeGuids failed to collect data.");
                return;
            }

            JObject attributesObj = JObject.FromObject (attributes);
            CommandResponse response = SendArchicadAddOnCommand ("GetLayerCombinations", attributesObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            LayerCombinationsObj layerCombinations = response.Result.ToObject<LayerCombinationsObj> ();
            List<string> attributeNames = new List<string> ();
            DataTree<AttributeIdItemObj> layerAttributeIds = new DataTree<AttributeIdItemObj> ();
            DataTree<bool> isHiddenLayers = new DataTree<bool> ();
            DataTree<bool> isLockedLayers = new DataTree<bool> ();
            DataTree<bool> isWireframeLayers = new DataTree<bool> ();
            DataTree<int> intersectionGroupsOfLayers = new DataTree<int> ();
            for (int i = 0; i < layerCombinations.LayerCombinations.Count; i++) {
                LayerCombinationObj layerCombination = layerCombinations.LayerCombinations[i];
                attributeNames.Add (layerCombination.LayerCombination.Name);
                List<AttributeIdItemObj> layerIds = new List<AttributeIdItemObj> ();
                List<bool> isHiddens = new List<bool> ();
                List<bool> isLockeds = new List<bool> ();
                List<bool> isWireframes = new List<bool> ();
                List<int> intersectionGroups = new List<int> ();
                foreach (ContainedLayerObj layer in layerCombination.LayerCombination.Layers) {
                    layerIds.Add (new AttributeIdItemObj () { AttributeId = layer.AttributeId });
                    isHiddens.Add (layer.IsHidden);
                    isLockeds.Add (layer.IsLocked);
                    isWireframes.Add (layer.IsWireframe);
                    intersectionGroups.Add (layer.IntersectionGroupNr);
                }
                layerAttributeIds.AddRange (layerIds, new GH_Path (i));
                isHiddenLayers.AddRange (isHiddens, new GH_Path (i));
                isLockedLayers.AddRange (isLockeds, new GH_Path (i));
                isWireframeLayers.AddRange (isWireframes, new GH_Path (i));
                intersectionGroupsOfLayers.AddRange (intersectionGroups, new GH_Path (i));
            }
            DA.SetDataList (0, attributeNames);
            DA.SetDataTree (1, layerAttributeIds);
            DA.SetDataTree (2, isHiddenLayers);
            DA.SetDataTree (3, isLockedLayers);
            DA.SetDataTree (4, isWireframeLayers);
            DA.SetDataTree (5, intersectionGroupsOfLayers);
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.LayerCombinations;

        public override Guid ComponentGuid => new Guid ("a4cebe10-f489-4cd6-a174-45b158a33365");
    }
}