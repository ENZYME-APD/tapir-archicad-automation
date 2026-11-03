using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetSurfaceQuantitiesOfElementsComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetSurfaceQuantitiesOfElements";

        public GetSurfaceQuantitiesOfElementsComponent()
            : base(
                "ElementSurfaceQuantities",
                "Gets the exposed surface areas of the given elements calculated by Archicad: for each " +
                "element one item per surface attribute and building material pair, with the area that " +
                "is not covered by the covering elements (e.g. the wall area behind a slab is taken off " +
                "the surface of the wall). Areas are in square meters.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "surfaceQuantitiesOfElements";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to get the surface quantities of.");

            InGenerics(
                "CoverElementGuids",
                "Identifiers of the covering elements: the area where one of them overlaps a surface " +
                "of an element is subtracted from that surface. Optional: every Wall, Column, Beam, " +
                "Slab, Roof, Shell, Morph, Mesh, Stair and Curtain Wall of the project by default. " +
                "An element never covers itself.");
            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "SurfaceGuids",
                "Surface attribute of each surface (one branch per element); null for a surface that " +
                "is not a project attribute, e.g. one defined by a GDL script.");

            OutGenericTree(
                "BuildingMaterialGuids",
                "Building material of the part of the element each surface belongs to (one branch " +
                "per element).");

            OutNumbers(
                "ExposedAreas",
                "Area of each surface that is not covered by any covering element, in square meters " +
                "(one branch per element).");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
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

            // Nothing connected leaves the default (every construction element) to the
            // add-on; a connected list names the covering elements.
            if (da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> coverWrappers)
                && coverWrappers.TryBuildObject(out ElementsObject coverElements))
            {
                parameters["coverElements"] = JObject.FromObject(coverElements)["elements"];
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
            var surfaces = new DataTree<object>();
            var materials = new DataTree<object>();
            var areas = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                surfaces.EnsurePath(path);
                materials.EnsurePath(path);
                areas.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (item["surfaces"] is JArray surfaceItems)
                {
                    foreach (var surface in surfaceItems)
                    {
                        surfaces.Add(
                            ToAttributeGuidWrapper(JsonOutputHelp.GuidOf(surface["surfaceId"]?["attributeId"])),
                            path);
                        materials.Add(
                            ToAttributeGuidWrapper(JsonOutputHelp.GuidOf(surface["buildingMaterialId"]?["attributeId"])),
                            path);
                        areas.Add(JsonOutputHelp.Scalar(surface, "exposedArea"), path);
                    }
                }
            }

            da.SetDataTree(0, surfaces);
            da.SetDataTree(1, materials);
            da.SetDataTree(2, areas);
            da.SetDataList(3, errors);
        }

        private static AttributeGuidWrapper ToAttributeGuidWrapper(
            string guid)
        {
            return guid == null
                ? null
                : new AttributeGuidWrapper
                {
                    AttributeId = new AttributeGuidObject { Guid = guid }
                };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetQuantitiesOfElements;

        public override Guid ComponentGuid =>
            new Guid("5b7c0e1a-2d94-4f3b-8a6e-c1d2f0a9b384");
    }
}
