using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetQuantitiesOfElementsComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetQuantitiesOfElements";

        public GetQuantitiesOfElementsComponent()
            : base(
                "ElementQuantities",
                "Gets the quantities of the given elements calculated by Archicad (volumes, surfaces, lengths " +
                "etc.), together with the volume and projected area per building material (one item per skin " +
                "of composite and complex profile elements, one item for basic elements). Supported types: " +
                "Wall, Column, Beam, Slab, Roof, Shell, Morph, Zone, ColumnSegment, BeamSegment. Lengths are " +
                "in meters, areas in square meters and volumes in cubic meters.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "quantitiesOfElements";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to get the quantities of.");

            inManager.AddNumberParameter(
                "MinOpeningSize",
                "MinOpeningSize",
                "Minimum opening size in square meters: openings smaller than this are not subtracted when " +
                "the conditional values (e.g. volumeConditional of a Wall) are calculated. Archicad only uses " +
                "it for walls. Optional, every opening is subtracted by default.",
                GH_ParamAccess.item);
            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "ElementTypes",
                "Type of each element.");

            OutTextTree(
                "QuantityNames",
                "Name of each quantity, e.g. volume or referenceSideSurface; they depend on the element " +
                "type (one branch per element).");

            OutNumbers(
                "QuantityValues",
                "Value of each quantity (one branch per element).");

            OutGenericTree(
                "SkinBuildingMaterialGuids",
                "Building material of each skin (one branch per element; empty for Zones and Morphs).");

            OutNumbers(
                "SkinVolumes",
                "Net volume of each skin (one branch per element).");

            OutNumbers(
                "SkinProjectedAreas",
                "Projected area of each skin (one branch per element). Archicad reports 0 when the legacy " +
                "intersection and surface calculation is turned on in the Project Preferences.");

            OutBooleanTree(
                "SkinIsCore",
                "Whether each skin is marked as core (one branch per element).");

            OutBooleanTree(
                "SkinIsFinish",
                "Whether each skin is marked as finish (one branch per element).");

            OutGenericTree(
                "SkinSubelementGuids",
                "The subelement (e.g. a column or beam segment) each skin belongs to, null when the skin " +
                "belongs to the element itself (one branch per element).");

            OutErrorMessages(
                "Error message of each element (empty when successful), e.g. for an unsupported type.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            var parameters = JObject.FromObject(elements);

            var minOpeningSize = 0.0;
            if (da.GetData(
                    1,
                    ref minOpeningSize))
            {
                if (minOpeningSize < 0.0)
                {
                    this.AddError("The input MinOpeningSize must not be negative.");
                    return;
                }
                parameters["minOpeningSize"] = minOpeningSize;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            SetOutputs(
                da,
                JsonOutputHelp.Items(
                    response,
                    ResponseArrayKey));
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var types = new List<object>();
            var quantityNames = new DataTree<object>();
            var quantityValues = new DataTree<object>();
            var materials = new DataTree<object>();
            var volumes = new DataTree<object>();
            var projectedAreas = new DataTree<object>();
            var isCore = new DataTree<object>();
            var isFinish = new DataTree<object>();
            var subelements = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                quantityNames.EnsurePath(path);
                quantityValues.EnsurePath(path);
                materials.EnsurePath(path);
                volumes.EnsurePath(path);
                projectedAreas.EnsurePath(path);
                isCore.EnsurePath(path);
                isFinish.EnsurePath(path);
                subelements.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    types.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                types.Add(JsonOutputHelp.Scalar(item, "type"));
                errors.Add("");

                if (item["quantities"] is JObject quantities)
                {
                    foreach (var quantity in quantities.Properties())
                    {
                        quantityNames.Add(quantity.Name, path);
                        quantityValues.Add(
                            quantity.Value.Type == JTokenType.Null
                                ? null
                                : (object)quantity.Value.Value<double>(),
                            path);
                    }
                }

                if (item["composites"] is JArray composites)
                {
                    foreach (var composite in composites)
                    {
                        var materialGuid = JsonOutputHelp.GuidOf(
                            composite["buildingMaterialId"]?["attributeId"]);
                        materials.Add(
                            materialGuid == null
                                ? null
                                : new AttributeGuidWrapper
                                {
                                    AttributeId = new AttributeGuidObject { Guid = materialGuid }
                                },
                            path);
                        volumes.Add(JsonOutputHelp.Scalar(composite, "volume"), path);
                        projectedAreas.Add(JsonOutputHelp.Scalar(composite, "projectedArea"), path);
                        isCore.Add(JsonOutputHelp.Scalar(composite, "isCore"), path);
                        isFinish.Add(JsonOutputHelp.Scalar(composite, "isFinish"), path);
                        subelements.Add(ElementIdOf(composite, "subElementId"), path);
                    }
                }
            }

            da.SetDataList(0, types);
            da.SetDataTree(1, quantityNames);
            da.SetDataTree(2, quantityValues);
            da.SetDataTree(3, materials);
            da.SetDataTree(4, volumes);
            da.SetDataTree(5, projectedAreas);
            da.SetDataTree(6, isCore);
            da.SetDataTree(7, isFinish);
            da.SetDataTree(8, subelements);
            da.SetDataList(9, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetQuantitiesOfElements;

        public override Guid ComponentGuid =>
            new Guid("78eed275-70ed-4a0b-b49e-94b35e75047e");
    }
}
