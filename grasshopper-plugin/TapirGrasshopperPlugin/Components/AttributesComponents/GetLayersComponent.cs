using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class LayerDetailsObj
    {
        [JsonProperty("name")]
        public string Name;

        [JsonProperty("isHidden")]
        public bool IsHidden;

        [JsonProperty("isLocked")]
        public bool IsLocked;

        [JsonProperty("isWireframe")]
        public bool IsWireframe;

        [JsonProperty("intersectionGroupNr")]
        public int IntersectionGroupNr;
    }

    public class LayerObj
    {
        [JsonProperty("layerAttribute")]
        public LayerDetailsObj LayerAttribute;
    }

    public class LayersObj
    {
        [JsonProperty("attributes")]
        public List<LayerObj> Attributes;
    }

    public class GetLayersComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Get the details of layers.";
        public override string CommandName => "GetLayerAttributes";

        public GetLayersComponent()
            : base(
                "Layers",
                "Layers",
                Doc,
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "List of layer attribute Guids.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Name",
                "Name of the layer.");

            OutBooleans(
                "IsHidden",
                "Visibility of the layer.");

            OutBooleans(
                "IsLocked",
                "Lock states of the layer.");

            OutBooleans(
                "IsWireframe",
                "Wireframe flag of the layer.");

            OutIntegers(
                "IntersectionGroup",
                "Intersection group of the layer.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var attributes = AttributeIdsObj.Create(
                da,
                0);
            if (attributes == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input AttributeGuids failed to collect data.");
                return;
            }

            var attributesObj = JObject.FromObject(attributes);

            if (!GetConvertedResponse(
                    CommandName,
                    attributesObj,
                    out LayersObj layers)) { return; }

            var name = new List<string>();
            var isHidden = new List<bool>();
            var isLocked = new List<bool>();
            var isWireframe = new List<bool>();
            var intersectionGroup = new List<int>();

            foreach (var layer in layers.Attributes)
            {
                name.Add(layer.LayerAttribute.Name);
                isHidden.Add(layer.LayerAttribute.IsHidden);
                isLocked.Add(layer.LayerAttribute.IsLocked);
                isWireframe.Add(layer.LayerAttribute.IsWireframe);
                intersectionGroup.Add(layer.LayerAttribute.IntersectionGroupNr);
            }

            da.SetDataList(
                0,
                name);
            da.SetDataList(
                1,
                isHidden);
            da.SetDataList(
                2,
                isLocked);
            da.SetDataList(
                3,
                isWireframe);
            da.SetDataList(
                4,
                intersectionGroup);
        }

        public override Guid ComponentGuid =>
            new Guid("0ffbee62-00a0-4974-9d9b-9bb1da20f6d0");
    }
}