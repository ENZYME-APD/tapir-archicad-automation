using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetBodyMaterialsOfElementsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetBodyMaterialsOfElements";

        public GetBodyMaterialsOfElementsComponent()
            : base(
                "GetBodyMaterialsOfElements",
                "Gets the building material of each body of the 3D model of the given elements. The 3D model " +
                "is generated with separate components in a temporary sight, so a composite structure has " +
                "one body per skin and the 3D window is not changed. The output is aligned with the input " +
                "list of elements. Available from Archicad 28.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "bodyMaterials";

        protected override void AddOutputs()
        {
            OutIntegerTree(
                "BodyIndices",
                "The index of each body of the element's 3D model (one branch per element).");

            OutGenericTree(
                "BuildingMaterialGuids",
                "The building material of each body, null when it has none (one branch per element).");

            OutTextTree(
                "BuildingMaterialNames",
                "The name of the building material of each body (one branch per element).");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        // A building material identifier of a response item, as an attribute
        // guid wrapper; null when absent.
        public static AttributeGuidWrapper BuildingMaterialOf(
            JToken item)
        {
            var guid = JsonOutputHelp.GuidOf(item?["buildingMaterialId"]);
            return guid == null
                ? null
                : new AttributeGuidWrapper
                {
                    AttributeId = new AttributeGuidObject { Guid = guid }
                };
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var bodyIndices = new DataTree<object>();
            var materials = new DataTree<object>();
            var materialNames = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                bodyIndices.EnsurePath(path);
                materials.EnsurePath(path);
                materialNames.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");
                if (!(item["bodies"] is JArray bodies))
                {
                    continue;
                }

                foreach (var body in bodies)
                {
                    bodyIndices.Add(
                        JsonOutputHelp.Scalar(
                            body,
                            "bodyIndex"),
                        path);
                    materials.Add(
                        BuildingMaterialOf(body),
                        path);
                    materialNames.Add(
                        JsonOutputHelp.Scalar(
                            body,
                            "buildingMaterialName"),
                        path);
                }
            }

            da.SetDataTree(
                0,
                bodyIndices);
            da.SetDataTree(
                1,
                materials);
            da.SetDataTree(
                2,
                materialNames);
            da.SetDataList(
                3,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetBodyMaterialsOfElements;

        public override Guid ComponentGuid =>
            new Guid("39c0b91c-8bed-4a85-8a72-3b6bd304f9ff");
    }
}
