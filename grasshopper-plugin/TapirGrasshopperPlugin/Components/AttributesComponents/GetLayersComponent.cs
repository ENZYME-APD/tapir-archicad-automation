using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
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

            if (!GetConvertedResponse(
                    CommandName,
                    attributes,
                    out LayersObj layers)) { return; }

            da.SetDataList(
                0,
                layers.Attributes.Select(x => x.LayerAttribute.Name));
            da.SetDataList(
                1,
                layers.Attributes.Select(x => x.LayerAttribute.IsHidden));
            da.SetDataList(
                2,
                layers.Attributes.Select(x => x.LayerAttribute.IsLocked));
            da.SetDataList(
                3,
                layers.Attributes.Select(x => x.LayerAttribute.IsWireframe));
            da.SetDataList(
                4,
                layers.Attributes.Select(x =>
                    x.LayerAttribute.IntersectionGroupNr));
        }

        public override Guid ComponentGuid =>
            new Guid("0ffbee62-00a0-4974-9d9b-9bb1da20f6d0");
    }
}