using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class TriangulatePolygonsOfElementsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "TriangulatePolygonsOfElements";

        public TriangulatePolygonsOfElementsComponent()
            : base(
                "TriangulatePolygonsOfElements",
                "Triangulates the plan polygon of the given elements, holes excluded, as Archicad does it: the " +
                "outline of a Slab, Zone, Hatch or Mesh and the polygon of a Roof. Each triangle is given by " +
                "the 2D coordinates of its three vertices. With Convex, the faces of the 3D model of each " +
                "element (any element type) are decomposed into convex polygons instead, as Archicad does it. " +
                "The output is aligned with the input list of elements; an element of another type gets an " +
                "error item.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "triangulations";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of Slabs, Zones, Hatches, Meshes or Roofs (any element with a 3D model with Convex).");

            InBoolean(
                "Convex",
                "Optional. True decomposes the faces of the 3D model of each element (its own bodies) into " +
                "convex polygons instead, returned in ConvexPolygons. The default is false: the plan polygon " +
                "is triangulated.");

            SetOptionality(1);
        }

        protected override bool TryAddParameters(
            IGH_DataAccess da,
            JObject parameters)
        {
            if (da.GetOptional(
                    1,
                    false))
            {
                parameters["convex"] = true;
            }

            return true;
        }

        protected override void AddOutputs()
        {
            OutCurveTree(
                "Triangles",
                "The triangles covering the polygon of each element as closed polylines on the XY plane " +
                "(one branch per element).");

            OutMeshes(
                "Meshes",
                "The triangles of each element joined into one mesh on the XY plane, or with Convex its " +
                "convex polygons joined into one mesh in 3D (null for an error).");

            OutErrorMessages(
                "Error message of each element (empty when successful).");

            OutCurveTree(
                "ConvexPolygons",
                "With Convex, the convex polygons of the faces of each element's 3D model as closed 3D " +
                "polylines in world coordinates (one branch per element).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var triangles = new DataTree<object>();
            var convexPolygons = new DataTree<object>();
            var meshes = new List<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                triangles.EnsurePath(path);
                convexPolygons.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    meshes.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");
                var mesh = new Mesh();
                if (item["triangles"] is JArray itemTriangles)
                {
                    foreach (var triangle in itemTriangles)
                    {
                        if (!(triangle["vertices"] is JArray vertices) ||
                            vertices.Count != 3)
                        {
                            continue;
                        }

                        triangles.Add(
                            ElementGeometryJsonHelp.Polyline2D(
                                vertices,
                                true),
                            path);

                        var first = mesh.Vertices.Count;
                        foreach (var vertex in vertices)
                        {
                            mesh.Vertices.Add(ElementGeometryJsonHelp.Point2D(vertex));
                        }
                        mesh.Faces.AddFace(
                            first,
                            first + 1,
                            first + 2);
                    }
                }

                if (item["convexPolygons"] is JArray itemPolygons)
                {
                    foreach (var polygon in itemPolygons)
                    {
                        if (!(polygon["vertices"] is JArray vertices) ||
                            vertices.Count < 3)
                        {
                            continue;
                        }

                        convexPolygons.Add(
                            ElementGeometryJsonHelp.Polyline3D(
                                vertices,
                                true),
                            path);

                        // A convex polygon is covered by the fan of triangles
                        // from its first vertex.
                        var first = mesh.Vertices.Count;
                        foreach (var vertex in vertices)
                        {
                            mesh.Vertices.Add(ElementGeometryJsonHelp.Point3D(vertex));
                        }
                        for (var v = 1; v < vertices.Count - 1; v++)
                        {
                            mesh.Faces.AddFace(
                                first,
                                first + v,
                                first + v + 1);
                        }
                    }
                }

                mesh.Vertices.CombineIdentical(
                    true,
                    true);
                mesh.Normals.ComputeNormals();
                meshes.Add(mesh);
            }

            da.SetDataTree(
                0,
                triangles);
            da.SetDataList(
                1,
                meshes);
            da.SetDataList(
                2,
                errors);
            da.SetDataTree(
                3,
                convexPolygons);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.TriangulatePolygonsOfElements;

        public override Guid ComponentGuid =>
            new Guid("f4a20f5f-36ec-431d-8c4a-b48b4e7b070d");
    }
}
