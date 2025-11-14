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
    public class LayerDataObj
    {
        [JsonProperty ("attributeId")]
        public AttributeIdObj AttributeId;

        [JsonProperty ("name")]
        public string Name;

        [JsonProperty ("isHidden")]
        public bool IsHidden;

        [JsonProperty ("isLocked")]
        public bool IsLocked;

        [JsonProperty ("isWireframe")]
        public bool IsWireframe;

        [JsonProperty ("intersectionGroupNr")]
        public int IntersectionGroupNr;
    }

    public class LayerDataArrayObj
    {
        [JsonProperty ("layerDataArray")]
        public List<LayerDataObj> LayerDataArray;

        [JsonProperty ("overwriteExisting")]
        public bool OverwriteExisting;
    }

    public class SetLayersComponent : ArchicadExecutorComponent
    {
        public SetLayersComponent ()
          : base (
                "Set Layers",
                "SetLayers",
                "Set the details of layers.",
                "Attributes"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("AttributeGuids", "AttributeGuids", "List of layer attribute Guids.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Name", "Name", "Name of the layer.", GH_ParamAccess.list);
            pManager.AddBooleanParameter ("IsHidden", "IsHidden", "Visibility of the layer.", GH_ParamAccess.list);
            pManager.AddBooleanParameter ("IsLocked", "IsLocked", "Lock states of the layer.", GH_ParamAccess.list);
            pManager.AddBooleanParameter ("IsWireframe", "IsWireframe", "Wireframe flag of the layer.", GH_ParamAccess.list);
            pManager.AddIntegerParameter ("IntersectionGroup", "IntersectionGroup", "Intersection group of the layer.", GH_ParamAccess.list);
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

            List<bool> isHiddenLayers = new List<bool> ();
            if (!DA.GetDataList (2, isHiddenLayers)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IsHidden failed to collect data.");
                return;
            }

            List<bool> isLockedLayers = new List<bool> ();
            if (!DA.GetDataList (3, isLockedLayers)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IsLocked failed to collect data.");
                return;
            }

            List<bool> isWireframeLayers = new List<bool> ();
            if (!DA.GetDataList (4, isWireframeLayers)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IsWireframe failed to collect data.");
                return;
            }

            List<int> intersectionGroupsOfLayers = new List<int> ();
            if (!DA.GetDataList (5, intersectionGroupsOfLayers)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IntersectionGroup failed to collect data.");
                return;
            }

            if ((names.Count != isHiddenLayers.Count && isHiddenLayers.Count != 1) ||
                (names.Count != isLockedLayers.Count && isLockedLayers.Count != 1) ||
                (names.Count != isWireframeLayers.Count && isWireframeLayers.Count != 1) ||
                (names.Count != intersectionGroupsOfLayers.Count && intersectionGroupsOfLayers.Count != 1)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "The number of items in the input lists must match the number of input layers.");
                return;
            }

            LayerDataArrayObj layerDataArray = new LayerDataArrayObj () {
                LayerDataArray = new List<LayerDataObj> (),
                OverwriteExisting = true
            };

            int index = 0;
            foreach (AttributeIdItemObj attributeId in attributes.Attributes) {
                layerDataArray.LayerDataArray.Add (new LayerDataObj () {
                    AttributeId = attributeId.AttributeId,
                    Name = names[index],
                    IsHidden = isHiddenLayers.Count == 1 ? isHiddenLayers[0] : isHiddenLayers[index],
                    IsLocked = isLockedLayers.Count == 1 ? isLockedLayers[0] : isLockedLayers[index],
                    IsWireframe = isWireframeLayers.Count == 1 ? isWireframeLayers[0] : isWireframeLayers[index],
                    IntersectionGroupNr = intersectionGroupsOfLayers.Count == 1 ? intersectionGroupsOfLayers[0] : intersectionGroupsOfLayers[index]
                });
                index++;
            }

            JObject layerDataArrayObj = JObject.FromObject (layerDataArray);
            CommandResponse response = SendArchicadAddOnCommand ("CreateLayers", layerDataArrayObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.SetLayers;

        public override Guid ComponentGuid => new Guid ("7e336988-3756-42e1-bc27-8d4e83e21ae5");
    }
}