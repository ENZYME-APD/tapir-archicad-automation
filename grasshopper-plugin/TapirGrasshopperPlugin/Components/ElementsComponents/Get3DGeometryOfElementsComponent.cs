using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class Get3DGeometryOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "Get3DGeometryOfElements";

        public Get3DGeometryOfElementsComponent()
            : base(
                "Element3DGeometry",
                "Gets the 3D geometry of elements from their Archicad 3D model as meshes.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to get the 3D geometry of.");
        }

        protected override void AddOutputs()
        {
            OutMeshes(
                "Meshes",
                "One mesh per element, in world coordinates, joined from every solid body of its 3D model " +
                "(the subelements of a Stair, Railing or Curtain Wall included). " +
                "Null for an element without a solid 3D model or with an error.");

            OutErrorMessages(
                "Error message of each element (empty when its geometry was read).");
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

            if (!TryGetConvertedCadValues(
                    CommandName,
                    elements,
                    ToAddOn,
                    JHelp.Deserialize<Geometries3DObject>,
                    out Geometries3DObject response))
            {
                return;
            }

            var meshes = new List<Mesh>();
            var errorMessages = new List<string>();
            foreach (var geometry in response.Geometries3D)
            {
                if (geometry?.Error != null)
                {
                    meshes.Add(null);
                    errorMessages.Add(geometry.Error.Message ?? "Unknown error.");
                    continue;
                }

                meshes.Add(ToMesh(geometry?.Bodies));
                errorMessages.Add("");
            }

            da.SetDataList(
                0,
                meshes);
            da.SetDataList(
                1,
                errorMessages);
        }

        // Joins the faces of every body into one mesh. Archicad's faces are
        // planar polygons with holes, not triangles, so each face is meshed
        // on its own and the pieces are joined; the shared vertices are
        // welded afterwards.
        private static Mesh ToMesh(
            List<Body3D> bodies)
        {
            if (bodies == null)
            {
                return null;
            }

            var mesh = new Mesh();
            foreach (var body in bodies)
            {
                if (body?.Vertices == null || body.Polygons == null)
                {
                    continue;
                }

                var vertices = body.Vertices
                    .Select(v => new Point3d(v.X, v.Y, v.Z))
                    .ToList();

                foreach (var polygon in body.Polygons)
                {
                    var faceMesh = ToFaceMesh(vertices, polygon);
                    if (faceMesh != null)
                    {
                        mesh.Append(faceMesh);
                    }
                }
            }

            if (mesh.Faces.Count == 0)
            {
                return null;
            }

            mesh.Vertices.CombineIdentical(true, true);
            mesh.Normals.ComputeNormals();
            mesh.Compact();
            return mesh;
        }

        private static Polyline ToClosedPolyline(
            IReadOnlyList<Point3d> vertices,
            List<int> vertexIds)
        {
            if (vertexIds == null || vertexIds.Count < 3)
            {
                return null;
            }

            var polyline = new Polyline();
            foreach (var vertexId in vertexIds)
            {
                if (vertexId < 0 || vertexId >= vertices.Count)
                {
                    return null;
                }
                polyline.Add(vertices[vertexId]);
            }
            polyline.Add(polyline[0]);
            return polyline;
        }

        private static Mesh ToFaceMesh(
            IReadOnlyList<Point3d> vertices,
            Polygon3D polygon)
        {
            var outline = ToClosedPolyline(vertices, polygon?.VertexIds);
            if (outline == null)
            {
                return null;
            }

            var holes = new List<Polyline>();
            if (polygon.Holes != null)
            {
                foreach (var hole in polygon.Holes)
                {
                    var holeOutline = ToClosedPolyline(vertices, hole?.VertexIds);
                    if (holeOutline != null)
                    {
                        holes.Add(holeOutline);
                    }
                }
            }

            if (holes.Count == 0)
            {
                // A triangle needs no triangulation; everything else (a quad
                // may be concave) goes through Rhino's planar polygon mesher.
                if (outline.Count == 4)
                {
                    var triangle = new Mesh();
                    triangle.Vertices.Add(outline[0]);
                    triangle.Vertices.Add(outline[1]);
                    triangle.Vertices.Add(outline[2]);
                    triangle.Faces.AddFace(0, 1, 2);
                    return triangle;
                }
                return Mesh.CreateFromClosedPolyline(outline);
            }

            // A face with holes is meshed through a planar Brep, which is
            // the one Rhino operation that keeps the holes.
            var loops = new List<Curve> { outline.ToPolylineCurve() };
            loops.AddRange(holes.Select(hole => (Curve)hole.ToPolylineCurve()));
            var breps = Brep.CreatePlanarBreps(loops, Tolerances.Main);
            if (breps == null || breps.Length == 0)
            {
                // Better a face without its holes than no face at all.
                return Mesh.CreateFromClosedPolyline(outline);
            }

            var faceMesh = new Mesh();
            foreach (var brep in breps)
            {
                var parts = Mesh.CreateFromBrep(brep, MeshingParameters.Default);
                if (parts == null)
                {
                    continue;
                }
                foreach (var part in parts)
                {
                    faceMesh.Append(part);
                }
            }
            return faceMesh.Faces.Count > 0
                ? faceMesh
                : Mesh.CreateFromClosedPolyline(outline);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Elems3DBoxes;

        public override Guid ComponentGuid =>
            new Guid("8a2f1c4e-7b3d-4e65-9f21-3c5d8e0a6b47");
    }
}
