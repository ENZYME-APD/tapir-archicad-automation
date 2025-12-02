using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetLayersComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLayerAttributes";

        public GetLayersComponent()
            : base(
                "Layers",
                "Get the details of layers.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics("LayerAttributeGuids");
        }

        protected override void AddOutputs()
        {
            OutTexts("LayerNames");

            OutBooleans(
                "IsHidden",
                "Visibility of the layers.");

            OutBooleans(
                "IsLocked",
                "Lock states of the layers.");

            OutBooleans(
                "IsWireframe",
                "Wireframe flag of the layers.");

            OutIntegers(
                "IntersectionGroup",
                "Intersection group of the layers.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out AttributeGuidItemsObject input))
            {
                return;
            }

            if (!TryGetConvertedValues(
                    CommandName,
                    input,
                    ToArchicad,
                    JHelp.Deserialize<LayersObj>,
                    out LayersObj response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Attributes.Select(x => x.LayerAttribute.Name));
            da.SetDataList(
                1,
                response.Attributes.Select(x => x.LayerAttribute.IsHidden));
            da.SetDataList(
                2,
                response.Attributes.Select(x => x.LayerAttribute.IsLocked));
            da.SetDataList(
                3,
                response.Attributes.Select(x => x.LayerAttribute.IsWireframe));
            da.SetDataList(
                4,
                response.Attributes.Select(x =>
                    x.LayerAttribute.IntersectionGroupNr));
        }

        public override Guid ComponentGuid =>
            new Guid("0ffbee62-00a0-4974-9d9b-9bb1da20f6d0");
    }
}