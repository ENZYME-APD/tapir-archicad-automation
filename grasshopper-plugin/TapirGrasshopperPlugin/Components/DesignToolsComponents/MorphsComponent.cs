using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Morph tool: reads, writes and creates morphs with the settings of
    // the Morph Settings dialog, from ToolSettings/Morph.json. The geometry
    // is given by plain inputs: Plane places a new morph (its origin is the
    // morph's base point, its axes the morph's orientation), and either Size
    // (a box, optionally widened at the top by TopExpansion) or Body (a mesh,
    // in world coordinates) gives its shape. ModifyMorphs takes no base point,
    // so Create always makes new morphs and Execute only writes the settings.
    public class MorphsComponent : ElementToolComponent
    {
        protected override string ToolName => "Morph";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateMorphs";
        protected override string CreateArrayKey => "morphsData";
        protected override string ModifyCommand => "ModifyMorphs";
        protected override string ModifyArrayKey => "morphsWithDetails";

        // The item properties of the CreateMorphs and ModifyMorphs input
        // schemas (ExtendedElementCommands.cpp); a setting the Modify command
        // does not take is written by Create, and the other way round.
        private static readonly ISet<string> CreateMorphsKeys = new HashSet<string>
        {
            "basePoint", "body", "buildingMaterialId", "castShadow", "coverFillBGPen", "coverFillOrientation",
            "coverFillPen", "coverFillType", "cutFillBackgroundPen", "cutFillPen", "cutLinePen", "cutLineType",
            "displayOption", "favoriteName", "floorIndex", "id", "isAutoOnStoryVisibility", "layerIndex", "level",
            "linkToSettings", "outlineContourDisplay", "overheadLinePen", "overheadLineType", "receiveShadow",
            "showContour", "showFill", "size", "surfaceId", "textureProjectionCoords", "textureProjectionType",
            "topExpansion", "uncutLinePen", "uncutLineType", "use3DHatching", "useCoverFillType",
            "useDistortedCoverFill", "viewDepthLimitation", "xAxis", "yAxis", "zAxis"
        };

        private static readonly ISet<string> ModifyMorphsKeys = new HashSet<string>
        {
            "body", "buildingMaterialId", "castShadow", "coverFillBGPen", "coverFillOrientation", "coverFillPen",
            "coverFillType", "cutFillBackgroundPen", "cutFillPen", "cutLinePen", "cutLineType", "displayOption",
            "isAutoOnStoryVisibility", "level", "linkToSettings", "outlineContourDisplay", "overheadLinePen",
            "overheadLineType", "receiveShadow", "rotationDegreesZ", "showContour", "showFill", "surfaceId",
            "textureProjectionCoords", "textureProjectionType", "translation", "uncutLinePen", "uncutLineType",
            "use3DHatching", "useCoverFillType", "useDistortedCoverFill", "viewDepthLimitation", "xAxis", "yAxis",
            "zAxis"
        };

        protected override ISet<string> CreateKeys => CreateMorphsKeys;
        protected override ISet<string> ModifyKeys => ModifyMorphsKeys;

        // The Morph Settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Geometry and Positioning",
            "Floor Plan and Section",
            "Model"
        };

        public MorphsComponent()
            : base(
                "Morphs",
                "Reads (Refresh), writes (Execute) and creates (Create) morphs, with the settings of the Morph " +
                "Settings dialog as one JSON object per dialog panel. A new morph starts from the Morph tool " +
                "defaults, or from FavoriteName, and the given settings override them; its geometry comes from " +
                "Plane and either Size (a box) or Body (a mesh). Execute writes the settings of existing morphs, " +
                "not their geometry.")
        {
        }

        protected override int GeometryInputCount => 4;

        protected override void AddGeometryInputs()
        {
            inManager.AddPlaneParameter(
                "Plane",
                "Plane",
                "Placement of each new morph (Create): its origin is the morph's base point, its axes the morph's " +
                "orientation. A point gives an unrotated morph at that point. May be left empty with Body, then " +
                "the morph is placed at the project origin. The home story is the story of the origin's " +
                "elevation unless Home Story is given in GeometryAndPositioning_JSON."
                    .WithTypeName("planeList"),
                GH_ParamAccess.list);
            InVectors(
                "Size",
                "Size of each new box morph along the X, Y and Z axes of its Plane (Create; all positive): one " +
                "for all or one per morph. Give Size or Body, not both.");
            InNumbers(
                "TopExpansion",
                "Widens the top of each new box morph outwards on all four sides by this amount, making a " +
                "frustum (Create, with Size only; 0 or more): one for all or one per morph.");
            inManager.AddMeshParameter(
                "Body",
                "Body",
                ("Shape of each new morph as a mesh, in world coordinates (Create): one for all or one per " +
                 "morph. A closed mesh becomes a solid morph, an open one a surface morph; each mesh face becomes " +
                 "one face of the morph. Give Size or Body, not both.")
                    .WithTypeName("meshList"),
                GH_ParamAccess.list);
        }

        protected override void AddGeometryOutputs()
        {
            outManager.AddPlaneParameter(
                "Plane",
                "Plane",
                "Placement of each morph: its origin is the morph's base point, its axes the morph's orientation."
                    .WithTypeName("planeList"),
                GH_ParamAccess.list);
            OutMeshes(
                "Body",
                "Shape of each morph as a mesh, in world coordinates (its filled faces).");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = new List<JObject>();
            var planes = new List<Plane>();
            var sizes = new List<Vector3d>();
            var topExpansions = new List<double>();
            var bodies = new List<Mesh>();
            da.GetDataList(firstInputIndex, planes);
            da.GetDataList(firstInputIndex + 1, sizes);
            da.GetDataList(firstInputIndex + 2, topExpansions);
            da.GetDataList(firstInputIndex + 3, bodies);

            if (planes.Count == 0 && sizes.Count == 0 && bodies.Count == 0)
            {
                // Nothing to create; the base class reports it.
                return true;
            }
            if (sizes.Count > 0 && bodies.Count > 0)
            {
                this.AddError("Give Size (a box) or Body (a mesh), not both.");
                return false;
            }
            if (sizes.Count == 0 && bodies.Count == 0)
            {
                this.AddError("Create needs the shape of the morphs: Size (a box) or Body (a mesh).");
                return false;
            }
            if (planes.Count == 0 && sizes.Count > 0)
            {
                this.AddError("Create needs Plane, the placement of each box morph.");
                return false;
            }
            if (topExpansions.Count > 0 && sizes.Count == 0)
            {
                this.AddError("TopExpansion is only used with Size.");
                return false;
            }

            var count = Math.Max(planes.Count, Math.Max(sizes.Count, bodies.Count));
            var lists = new[]
            {
                new KeyValuePair<string, int>("Plane", planes.Count),
                new KeyValuePair<string, int>("Size", sizes.Count),
                new KeyValuePair<string, int>("TopExpansion", topExpansions.Count),
                new KeyValuePair<string, int>("Body", bodies.Count)
            };
            foreach (var list in lists)
            {
                if (list.Value > 1 && list.Value != count)
                {
                    this.AddError(
                        $"{list.Key} has {list.Value} items: give none, one for all {count} morphs, or one per morph.");
                    return false;
                }
            }

            for (var i = 0; i < count; i++)
            {
                var plane = planes.Count == 0 ? Plane.WorldXY : ItemAt(planes, i);
                if (!plane.IsValid)
                {
                    this.AddError($"Plane item {i} is not a valid plane.");
                    return false;
                }

                // Maps the morph's local coordinates to world coordinates.
                // Its rows are the xAxis, yAxis and zAxis of CreateMorphs
                // (the rows of the morph's tranmat), its last column the
                // base point.
                var toWorld = Transform.PlaneToPlane(Plane.WorldXY, plane);
                var item = new JObject
                {
                    ["basePoint"] = Coordinate(plane.Origin.X, plane.Origin.Y, plane.Origin.Z),
                    ["xAxis"] = Coordinate(toWorld.M00, toWorld.M01, toWorld.M02),
                    ["yAxis"] = Coordinate(toWorld.M10, toWorld.M11, toWorld.M12),
                    ["zAxis"] = Coordinate(toWorld.M20, toWorld.M21, toWorld.M22)
                };

                if (sizes.Count > 0)
                {
                    var size = ItemAt(sizes, i);
                    if (!(size.X > 0 && size.Y > 0 && size.Z > 0))
                    {
                        this.AddError($"Size item {i} must be positive along X, Y and Z.");
                        return false;
                    }
                    item["size"] = Coordinate(size.X, size.Y, size.Z);

                    if (topExpansions.Count > 0)
                    {
                        var topExpansion = ItemAt(topExpansions, i);
                        if (!(topExpansion >= 0))
                        {
                            this.AddError($"TopExpansion item {i} must be 0 or more.");
                            return false;
                        }
                        item["topExpansion"] = topExpansion;
                    }
                }
                else
                {
                    var mesh = ItemAt(bodies, i);
                    if (mesh == null || !mesh.IsValid || mesh.Faces.Count == 0)
                    {
                        this.AddError($"Body item {i} is not a valid mesh with faces.");
                        return false;
                    }
                    if (!toWorld.TryGetInverse(out var toLocal))
                    {
                        this.AddError($"Plane item {i} cannot be used to place a mesh.");
                        return false;
                    }
                    item["body"] = BodyOf(mesh, toLocal);
                }

                geometry.Add(item);
            }

            return true;
        }

        // The MorphBody JSON of a mesh: its topology vertices in the morph's
        // local coordinates and one polygon per mesh face, counterclockwise
        // seen from outside.
        private static JObject BodyOf(
            Mesh source,
            Transform toLocal)
        {
            var mesh = source.DuplicateMesh();
            if (mesh.IsClosed && mesh.SolidOrientation() == -1)
            {
                // Faces pointing inwards: Archicad expects them outwards.
                mesh.Flip(true, true, true);
            }

            var topology = mesh.TopologyVertices;
            var vertices = new JArray();
            for (var v = 0; v < topology.Count; v++)
            {
                Point3d point = topology[v];
                point.Transform(toLocal);
                vertices.Add(Coordinate(point.X, point.Y, point.Z));
            }

            var polygons = new JArray();
            foreach (var face in mesh.Faces)
            {
                var corners = face.IsQuad
                    ? new[] { face.A, face.B, face.C, face.D }
                    : new[] { face.A, face.B, face.C };
                var vertexIds = new List<int>();
                foreach (var corner in corners)
                {
                    var id = topology.TopologyVertexIndex(corner);
                    if (!vertexIds.Contains(id))
                    {
                        vertexIds.Add(id);
                    }
                }
                if (vertexIds.Count < 3)
                {
                    // A degenerate face.
                    continue;
                }
                polygons.Add(new JObject { ["vertexIds"] = new JArray(vertexIds) });
            }

            return new JObject
            {
                ["bodyType"] = mesh.IsClosed ? "Solid" : "Surface",
                ["vertices"] = vertices,
                ["polygons"] = polygons
            };
        }

        protected override void SetGeometryOutputs(
            IGH_DataAccess da,
            int firstOutputIndex,
            List<JObject> details)
        {
            var transforms = details.Select(TransformOf).ToList();
            da.SetDataList(
                firstOutputIndex,
                transforms.Select(
                    t =>
                    {
                        if (t == null)
                        {
                            return (Plane?) null;
                        }
                        var plane = Plane.WorldXY;
                        plane.Transform(t.Value);
                        return plane;
                    }));
            da.SetDataList(
                firstOutputIndex + 1,
                details.Select((d, i) => MeshOf(d, transforms[i])));
        }

        // The morph's local-to-world transform from GetDetailsOfElements:
        // origin is the translation, xAxis, yAxis and zAxis the rows of the
        // rotation part of its tranmat.
        private static Transform? TransformOf(
            JObject details)
        {
            if (!(details?["origin"] is JObject origin) ||
                !(details["xAxis"] is JObject xAxis) ||
                !(details["yAxis"] is JObject yAxis) ||
                !(details["zAxis"] is JObject zAxis))
            {
                return null;
            }

            var transform = Transform.Identity;
            transform.M00 = (double) xAxis["x"];
            transform.M01 = (double) xAxis["y"];
            transform.M02 = (double) xAxis["z"];
            transform.M03 = (double) origin["x"];
            transform.M10 = (double) yAxis["x"];
            transform.M11 = (double) yAxis["y"];
            transform.M12 = (double) yAxis["z"];
            transform.M13 = (double) origin["y"];
            transform.M20 = (double) zAxis["x"];
            transform.M21 = (double) zAxis["y"];
            transform.M22 = (double) zAxis["z"];
            transform.M23 = (double) origin["z"];
            return transform;
        }

        private static Mesh MeshOf(
            JObject details,
            Transform? toWorld)
        {
            if (toWorld == null ||
                !(details?["body"] is JObject body) ||
                !(body["vertices"] is JArray vertexArray) ||
                !(body["polygons"] is JArray polygons))
            {
                return null;
            }

            var vertices = new List<Point3d>();
            foreach (var vertex in vertexArray)
            {
                var point = new Point3d((double) vertex["x"], (double) vertex["y"], (double) vertex["z"]);
                point.Transform(toWorld.Value);
                vertices.Add(point);
            }

            var mesh = new Mesh();
            foreach (var polygon in polygons.OfType<JObject>())
            {
                // An unfilled polygon is only an outline, with no face.
                if (polygon["filled"]?.Type == JTokenType.Boolean && !(bool) polygon["filled"])
                {
                    continue;
                }
                var faceMesh = Get3DGeometryOfElementsComponent.ToFaceMesh(vertices, polygon.ToObject<Polygon3D>());
                if (faceMesh != null)
                {
                    mesh.Append(faceMesh);
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

        private static JObject Coordinate(
            double x,
            double y,
            double z)
        {
            return new JObject { ["x"] = x, ["y"] = y, ["z"] = z };
        }

        private static T ItemAt<T>(
            List<T> list,
            int i)
        {
            return list.Count == 1 ? list[0] : list[i];
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMorphs;

        public override Guid ComponentGuid =>
            new Guid("6d3f9a2e-4b81-4c57-9e0a-7f2c5b8d1e43");
    }
}
