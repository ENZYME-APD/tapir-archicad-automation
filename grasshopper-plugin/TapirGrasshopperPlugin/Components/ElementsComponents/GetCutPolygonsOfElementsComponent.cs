using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using Rhino;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetCutPolygonsOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCutPolygonsOfElements";

        public GetCutPolygonsOfElementsComponent()
            : base(
                "GetCutPolygonsOfElements",
                "Cuts each body of the 3D model of the given elements with a plane and returns, for each body, " +
                "its building material, the area of the cut surface and its polygons (outline and holes). " +
                "The 3D model is generated with separate components in a temporary sight, so a composite " +
                "structure has one body per skin and the 3D window is not changed. The output is aligned " +
                "with the input list of elements. Available from Archicad 28.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to cut.");

            inManager.AddPlaneParameter(
                "CutPlane",
                "CutPlane",
                "The cutting plane: its origin is the base point, its X, Y and Z axes are axisX, axisY and " +
                "axisZ (the normal of the plane)."
                    .WithTypeName("planeItem"),
                GH_ParamAccess.item);
        }

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

            OutNumbers(
                "Areas",
                "The area of the cut surface of each body (one branch per element).");

            OutCurveTree(
                "Outlines",
                "The outline of each polygon of the cut surface, as a closed polyline on the cutting plane in " +
                "world coordinates; curved edges are given by their end points (one branch per element and " +
                "body).");

            OutCurveTree(
                "Holes",
                "The holes of each polygon of the cut surface, as closed polylines on the cutting plane in " +
                "world coordinates (one branch per element, body and polygon).");

            OutGenericTree(
                "Regions",
                "Each polygon of the cut surface with its holes as a planar surface (one branch per element " +
                "and body).");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        private static JObject CoordinateOf(
            Point3d point)
        {
            return new JObject
            {
                ["x"] = point.X,
                ["y"] = point.Y,
                ["z"] = point.Z
            };
        }

        private static JObject CoordinateOf(
            Vector3d vector)
        {
            return CoordinateOf(new Point3d(vector));
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements) ||
                !da.TryGet(
                    1,
                    out Plane plane))
            {
                return;
            }

            if (!plane.IsValid)
            {
                this.AddError("The input CutPlane is not a valid plane.");
                return;
            }

            var parameters = JObject.FromObject(elements);
            parameters["cutPlane"] = new JObject
            {
                ["basePoint"] = CoordinateOf(plane.Origin),
                ["axisX"] = CoordinateOf(plane.XAxis),
                ["axisY"] = CoordinateOf(plane.YAxis),
                ["axisZ"] = CoordinateOf(plane.ZAxis)
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var bodyIndices = new DataTree<object>();
            var materials = new DataTree<object>();
            var materialNames = new DataTree<object>();
            var areas = new DataTree<object>();
            var outlines = new DataTree<object>();
            var holes = new DataTree<object>();
            var regions = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "cutPolygons");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                bodyIndices.EnsurePath(path);
                materials.EnsurePath(path);
                materialNames.EnsurePath(path);
                areas.EnsurePath(path);

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

                for (var j = 0; j < bodies.Count; j++)
                {
                    var body = bodies[j];
                    var bodyPath = new GH_Path(
                        i,
                        j);
                    outlines.EnsurePath(bodyPath);
                    regions.EnsurePath(bodyPath);

                    bodyIndices.Add(
                        JsonOutputHelp.Scalar(
                            body,
                            "bodyIndex"),
                        path);
                    materials.Add(
                        GetBodyMaterialsOfElementsComponent.BuildingMaterialOf(body),
                        path);
                    materialNames.Add(
                        JsonOutputHelp.Scalar(
                            body,
                            "buildingMaterialName"),
                        path);
                    areas.Add(
                        JsonOutputHelp.Scalar(
                            body,
                            "area"),
                        path);

                    if (!(body["polygons"] is JArray polygons))
                    {
                        continue;
                    }

                    for (var k = 0; k < polygons.Count; k++)
                    {
                        var polygon = polygons[k];
                        var holesPath = new GH_Path(
                            i,
                            j,
                            k);
                        holes.EnsurePath(holesPath);

                        var outline = ElementGeometryJsonHelp.Polyline2D(
                            polygon["polygonOutline"],
                            true,
                            plane);
                        outlines.Add(
                            outline,
                            bodyPath);

                        var holeCurves = new List<Curve>();
                        if (polygon["holes"] is JArray polygonHoles)
                        {
                            foreach (var hole in polygonHoles)
                            {
                                var holeCurve = ElementGeometryJsonHelp.Polyline2D(
                                    hole["polygonOutline"] ?? hole["polygonCoordinates"],
                                    true,
                                    plane);
                                holes.Add(
                                    holeCurve,
                                    holesPath);
                                if (holeCurve != null)
                                {
                                    holeCurves.Add(holeCurve);
                                }
                            }
                        }

                        Brep region = null;
                        if (outline != null)
                        {
                            var breps = Brep.CreatePlanarBreps(
                                new List<Curve> { outline }.Concat(holeCurves),
                                RhinoMath.SqrtEpsilon);
                            if (breps != null && breps.Length == 1)
                            {
                                region = breps[0];
                            }
                        }
                        regions.Add(
                            region,
                            bodyPath);
                    }
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
            da.SetDataTree(
                3,
                areas);
            da.SetDataTree(
                4,
                outlines);
            da.SetDataTree(
                5,
                holes);
            da.SetDataTree(
                6,
                regions);
            da.SetDataList(
                7,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCutPolygonsOfElements;

        public override Guid ComponentGuid =>
            new Guid("aa7871d3-9f17-4420-a54c-8269233de3a1");
    }
}
