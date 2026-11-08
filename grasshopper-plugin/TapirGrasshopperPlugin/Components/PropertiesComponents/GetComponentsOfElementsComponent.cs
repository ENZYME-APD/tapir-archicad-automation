using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetComponentsOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetComponentsOfElements";

        public GetComponentsOfElementsComponent()
            : base(
                "GetComponentsOfElements",
                "Get the components of the elements: the building material skins of composite walls, slabs, " +
                "roofs and shells, and the parts of complex profile walls, beams and columns. The components " +
                "have their own property values (building material, thickness, area, volume and the " +
                "properties of the building material); read them with GetPropertyValuesOfElementComponents. " +
                "The outputs have one branch per queried element.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ElementGuids",
                "The element of each component (one branch per queried element), to pass on with " +
                "ComponentGuids.");

            OutTextTree(
                "ComponentGuids",
                "The components of each element (one branch per queried element).");

            OutErrorMessages(
                "Error message for each queried element (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out List<JObject> elementIds))
            {
                return;
            }

            var elements = new JArray();
            foreach (var elementId in elementIds)
            {
                elements.Add(new JObject { ["elementId"] = elementId });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["elements"] = elements },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elementGuids = new DataTree<object>();
            var componentGuids = new DataTree<string>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "componentsOfElements");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                elementGuids.EnsurePath(path);
                componentGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                foreach (var component in JsonOutputHelp.Items(
                             item as JObject,
                             "elementComponents"))
                {
                    var elementComponentId = component?["elementComponentId"];
                    elementGuids.Add(
                        ItemListHelp.ElementGuidOf(elementComponentId?["elementId"]),
                        path);
                    componentGuids.Add(
                        JsonOutputHelp.GuidOf(elementComponentId?["componentId"]),
                        path);
                }
            }

            da.SetDataTree(0, elementGuids);
            da.SetDataTree(1, componentGuids);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetComponentsOfElements;

        public override Guid ComponentGuid =>
            new Guid("4445e03a-876e-4de7-8458-1ca3a94b75f9");
    }
}
