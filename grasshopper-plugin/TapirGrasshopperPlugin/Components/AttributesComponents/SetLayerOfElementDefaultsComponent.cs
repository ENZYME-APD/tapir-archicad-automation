using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class SetLayerOfElementDefaultsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetLayerOfElementDefaults";

        public SetLayerOfElementDefaultsComponent()
            : base(
                "SetLayerOfElementDefaults",
                "Sets the layer of the tool defaults of the given element types, so new elements of that type are " +
                "placed on it. Only the layer of the defaults is changed.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ElementTypes",
                "Element types whose tool defaults get the layer.");

            InGenerics(
                "LayerGuids",
                "The layer of each element type (input only 1 to use the same layer for all).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element type (empty when it succeeded).");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> elementTypes))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> layerWrappers))
            {
                return;
            }

            if (layerWrappers.Count != 1 &&
                layerWrappers.Count != elementTypes.Count)
            {
                this.AddError(
                    "The count of LayerGuids must be 1 or the same as the count of ElementTypes.");
                return;
            }

            var layerIds = new List<AttributeGuidObject>();
            foreach (var wrapper in layerWrappers)
            {
                var layerId = GuidObject<AttributeGuidObject>.CreateFromWrapper(wrapper);
                if (layerId == null)
                {
                    this.AddError(
                        "Invalid layer identifier in the LayerGuids input.");
                    return;
                }

                layerIds.Add(layerId);
            }

            var items = new JArray();
            for (var i = 0; i < elementTypes.Count; i++)
            {
                var layerId = layerIds.Count == 1
                    ? layerIds[0]
                    : layerIds[i];
                items.Add(
                    new JObject
                    {
                        ["elementType"] = elementTypes[i],
                        ["layerId"] = new JObject { ["guid"] = layerId.Guid }
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["layersOfElementDefaults"] = items },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLayerOfElementDefaults;

        public override Guid ComponentGuid =>
            new Guid("47787acb-adc1-42a8-ad4a-9309c148bd32");
    }
}
