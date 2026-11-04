using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetSurfaceTypesOfElementsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetSurfaceTypesOfElements";

        public GetSurfaceTypesOfElementsComponent()
            : base(
                "GetSurfaceTypesOfElements",
                "Gets the surface type of every polygon of the 3D model of the given elements, as " +
                "ACAPI_Polygon_PolygonId2SurfaceType returns it: for each element its polygons with their " +
                "body index, polygon index, polygon id and the numeric API_SurfaceTypeID value. The model " +
                "is generated with separate components in a temporary sight, so the 3D window is not " +
                "changed. The outputs are aligned with the input list of elements, one branch per element. " +
                "Available from Archicad 28.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "surfaceTypes";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements.");
        }

        protected override void AddOutputs()
        {
            OutIntegerTree(
                "BodyIndices",
                "The index of the body of each polygon in the element, starting from 0 (one branch per element).");

            OutIntegerTree(
                "PolygonIndices",
                "The index of each polygon in its body, starting from 0 (one branch per element).");

            OutIntegerTree(
                "PolygonIds",
                "The identifier of each polygon in the 3D model (one branch per element).");

            OutIntegerTree(
                "SurfaceTypes",
                "The surface type of each polygon: the numeric value of API_SurfaceTypeID (one branch per element).");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var bodyIndices = new DataTree<object>();
            var polygonIndices = new DataTree<object>();
            var polygonIds = new DataTree<object>();
            var surfaceTypes = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                bodyIndices.EnsurePath(path);
                polygonIndices.EnsurePath(path);
                polygonIds.EnsurePath(path);
                surfaceTypes.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");
                if (!(item["polygons"] is JArray polygons))
                {
                    continue;
                }

                foreach (var polygon in polygons)
                {
                    bodyIndices.Add(
                        polygon.Value<int?>("bodyIndex"),
                        path);
                    polygonIndices.Add(
                        polygon.Value<int?>("polygonIndex"),
                        path);
                    polygonIds.Add(
                        polygon.Value<int?>("polygonId"),
                        path);
                    surfaceTypes.Add(
                        polygon.Value<int?>("surfaceType"),
                        path);
                }
            }

            da.SetDataTree(
                0,
                bodyIndices);
            da.SetDataTree(
                1,
                polygonIndices);
            da.SetDataTree(
                2,
                polygonIds);
            da.SetDataTree(
                3,
                surfaceTypes);
            da.SetDataList(
                4,
                errors);
        }

        // Shares the icon of GetBodyMaterialsOfElements until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetBodyMaterialsOfElements;

        public override Guid ComponentGuid =>
            new Guid("276abc9a-c84e-4d72-952c-8e97b972aabe");
    }
}
