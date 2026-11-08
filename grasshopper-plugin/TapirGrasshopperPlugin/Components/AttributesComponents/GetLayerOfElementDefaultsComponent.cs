using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetLayerOfElementDefaultsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLayerOfElementDefaults";

        public GetLayerOfElementDefaultsComponent()
            : base(
                "GetLayerOfElementDefaults",
                "Returns the layer of the tool defaults of the given element types: the layer new elements of " +
                "that type are placed on.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ElementTypes",
                "Element types whose tool defaults to read.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "LayerGuids",
                "The layer of the tool defaults of each element type (null when it could not be read).");

            OutTexts(
                "LayerNames",
                "The name of that layer.");

            OutErrorMessages(
                "Error message of each element type (empty when successful), e.g. when the type has no tool defaults.");
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

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["elementTypes"] = JArray.FromObject(elementTypes) },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var layerGuids = new List<AttributeGuidWrapper>();
            var layerNames = new List<string>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "layersOfElementDefaults"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    layerGuids.Add(null);
                    layerNames.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                var guid = JsonOutputHelp.GuidOf(item["layerId"]);
                layerGuids.Add(
                    guid == null
                        ? null
                        : new AttributeGuidWrapper
                        {
                            AttributeId = new AttributeGuidObject { Guid = guid }
                        });
                layerNames.Add(item["layerName"]?.ToString());
                errors.Add("");
            }

            da.SetDataList(0, layerGuids);
            da.SetDataList(1, layerNames);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLayerOfElementDefaults;

        public override Guid ComponentGuid =>
            new Guid("3778e9a5-8eaf-41c7-ab11-d2b15e8658b7");
    }
}
