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
                "the 2D coordinates of its three vertices. The output is aligned with the input list of " +
                "elements; an element of another type gets an error item.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "triangulations";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of Slabs, Zones, Hatches, Meshes or Roofs.");
        }

        protected override void AddOutputs()
        {
            OutCurveTree(
                "Triangles",
                "The triangles covering the polygon of each element as closed polylines on the XY plane " +
                "(one branch per element).");

            OutMeshes(
                "Meshes",
                "The triangles of each element joined into one mesh on the XY plane (null for an error).");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var triangles = new DataTree<object>();
            var meshes = new List<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                triangles.EnsurePath(path);

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
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.TriangulatePolygonsOfElements;

        public override Guid ComponentGuid =>
            new Guid("f4a20f5f-36ec-431d-8c4a-b48b4e7b070d");
    }
}
