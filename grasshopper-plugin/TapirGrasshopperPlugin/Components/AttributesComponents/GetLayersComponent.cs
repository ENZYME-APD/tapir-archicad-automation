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
    public class LayerDetailsObj
    {
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

    public class LayerObj
    {
        [JsonProperty ("layerAttribute")]
        public LayerDetailsObj LayerAttribute;
    }

    public class LayersObj
    {
        [JsonProperty ("attributes")]
        public List<LayerObj> Attributes;
    }

    public class GetLayersComponent : ArchicadAccessorComponent
    {
        public GetLayersComponent ()
          : base (
                "Layers",
                "Layers",
                "Get the details of layers.",
                "Attributes"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("AttributeGuids", "AttributeGuids", "List of layer attribute Guids.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter ("Name", "Name", "Name of the layer.", GH_ParamAccess.list);
            pManager.AddBooleanParameter ("IsHidden", "IsHidden", "Visibility of the layer.", GH_ParamAccess.list);
            pManager.AddBooleanParameter ("IsLocked", "IsLocked", "Lock states of the layer.", GH_ParamAccess.list);
            pManager.AddBooleanParameter ("IsWireframe", "IsWireframe", "Wireframe flag of the layer.", GH_ParamAccess.list);
            pManager.AddIntegerParameter ("IntersectionGroup", "IntersectionGroup", "Intersection group of the layer.", GH_ParamAccess.list);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            AttributeIdsObj attributes = AttributeIdsObj.Create (DA, 0);
            if (attributes == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input AttributeGuids failed to collect data.");
                return;
            }

            JObject attributesObj = JObject.FromObject (attributes);
            CommandResponse response = SendArchicadCommand ("GetLayerAttributes", attributesObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            LayersObj layers = response.Result.ToObject<LayersObj> ();
            List<string> name = new List<string> ();
            List<bool> isHidden = new List<bool> ();
            List<bool> isLocked = new List<bool> ();
            List<bool> isWireframe = new List<bool> ();
            List<int> intersectionGroup = new List<int> ();
            foreach (LayerObj layer in layers.Attributes) {
                name.Add (layer.LayerAttribute.Name);
                isHidden.Add (layer.LayerAttribute.IsHidden);
                isLocked.Add (layer.LayerAttribute.IsLocked);
                isWireframe.Add (layer.LayerAttribute.IsWireframe);
                intersectionGroup.Add (layer.LayerAttribute.IntersectionGroupNr);
            }
            DA.SetDataList (0, name);
            DA.SetDataList (1, isHidden);
            DA.SetDataList (2, isLocked);
            DA.SetDataList (3, isWireframe);
            DA.SetDataList (4, intersectionGroup);
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.Layers;

        public override Guid ComponentGuid => new Guid ("0ffbee62-00a0-4974-9d9b-9bb1da20f6d0");
    }
}