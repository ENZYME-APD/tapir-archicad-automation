using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetCutPolygonsOfElementsComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetCutPolygonsOfElements";

        public GetCutPolygonsOfElementsComponent()
            : base(
                "ElementCutPolygons",
                "Cuts the 3D model of elements with a horizontal plane and gets the cut polygons and " +
                "their total area, the way the floor plan cuts them: at the given height, or by default " +
                "at the floor plan cut plane of each element's home story. Available from Archicad 27.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "cutPolygonsOfElements";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to cut.");

            inManager.AddNumberParameter(
                "CutHeight",
                "CutHeight",
                "Absolute height (Z from the project zero, in meters) of the horizontal cutting plane. " +
                "Optional: by default every element is cut at the floor plan cut plane of its home story.",
                GH_ParamAccess.item);
            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutCurveTree(
                "Polygons",
                "The cut polygons as closed curves at the cut height, path {element; polygon}: the outline " +
                "first, then its holes. An empty branch for an element the plane misses or with an error.");

            OutNumberList(
                "Areas",
                "Total area of the cut polygons of each element in square meters (null for an error).");

            OutNumberList(
                "CutHeights",
                "The absolute height each element was cut at (null for an error).");

            OutErrorMessages(
                "Error message of each element (empty when it was cut).");
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

            var cutHeight = 0.0;
            if (da.GetData(
                    1,
                    ref cutHeight))
            {
                parameters["cutHeight"] = cutHeight;
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
            var polygons = new DataTree<object>();
            var areas = new List<object>();
            var cutHeights = new List<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                polygons.EnsurePath(new GH_Path(i));

                if (JsonOutputHelp.IsError(item))
                {
                    areas.Add(null);
                    cutHeights.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                var cutHeightOfElement = item.Value<double?>("cutHeight") ?? 0.0;
                areas.Add(JsonOutputHelp.Scalar(item, "area"));
                cutHeights.Add(cutHeightOfElement);
                errors.Add("");

                if (!(item["polygons"] is JArray cutPolygons))
                {
                    continue;
                }

                for (var j = 0; j < cutPolygons.Count; j++)
                {
                    var path = new GH_Path(i, j);
                    polygons.EnsurePath(path);
                    AddCurve(polygons, path, cutPolygons[j]?["polygonOutline"], cutHeightOfElement);
                    if (cutPolygons[j]?["holes"] is JArray holes)
                    {
                        foreach (var hole in holes)
                        {
                            AddCurve(polygons, path, hole?["polygonOutline"], cutHeightOfElement);
                        }
                    }
                }
            }

            da.SetDataTree(0, polygons);
            da.SetDataList(1, areas);
            da.SetDataList(2, cutHeights);
            da.SetDataList(3, errors);
        }

        // The outline comes closed (its last coordinate repeats the first), so the
        // polyline is taken as is.
        private static void AddCurve(
            DataTree<object> curves,
            GH_Path path,
            JToken outline,
            double z)
        {
            if (!(outline is JArray coordinates) || coordinates.Count < 2)
            {
                return;
            }

            var polyline = new Polyline();
            foreach (var coordinate in coordinates)
            {
                polyline.Add(new Point3d(
                    coordinate.Value<double?>("x") ?? 0.0,
                    coordinate.Value<double?>("y") ?? 0.0,
                    z));
            }
            curves.Add(polyline.ToPolylineCurve(), path);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Elems3DBoxes;

        public override Guid ComponentGuid =>
            new Guid("c4d7e2a1-5b8f-4c39-9e6d-2f7a1b3c8d05");
    }
}
