using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Mesh tool: reads, writes and creates meshes with the settings of
    // the Mesh Settings dialog, from ToolSettings/Mesh.json. Polygon defines
    // a new mesh: a closed polyline whose vertex Z is the height of the
    // vertex above the Mesh Reference Plane (Offset to Home Story).
    public class MeshesComponent : ElementToolComponent
    {
        protected override string ToolName => "Mesh";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateMeshes";
        protected override string CreateArrayKey => "meshesData";
        protected override string ModifyCommand => "ModifyMeshes";
        protected override string ModifyArrayKey => "meshesData";

        // ModifyMeshes takes the settings of an item under meshData.
        protected override string ModifyItemSettingsKey => "meshData";

        // The item properties of the CreateMeshes input schema
        // (ElementCreationCommands.cpp) and of ModifyMeshes' meshData
        // (MeshModificationData); a setting the Modify command does not take
        // is written by Create, and the other way round.
        private static readonly ISet<string> CreateMeshesKeys = new HashSet<string>
        {
            "buildingMaterialId", "contourPen", "favoriteName", "floorIndex", "holes", "id", "level",
            "levelPen", "lineTypeIndex", "polygonArcs", "polygonCoordinates", "ridges", "showLines",
            "skirtLevel", "skirtType", "sublines"
        };

        private static readonly ISet<string> ModifyMeshesKeys = new HashSet<string>
        {
            "contourPen", "floorIndex", "holes", "level", "levelPen", "lineTypeIndex", "polygonArcs",
            "polygonCoordinates", "ridges", "showLines", "skirtLevel", "skirtType", "sublines"
        };

        // ModifyMeshes takes the polygon, so Create updates the meshes this
        // component made before instead of making new ones.
        protected override bool CanUpdateCreatedElements => true;

        protected override ISet<string> CreateKeys => CreateMeshesKeys;
        protected override ISet<string> ModifyKeys => ModifyMeshesKeys;

        // The Mesh Settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Geometry and Positioning",
            "Floor Plan and Section",
            "Model"
        };

        public MeshesComponent()
            : base(
                "Meshes",
                "Reads (Refresh), writes (Execute) and creates (Create) meshes, with the settings of the " +
                "Mesh Settings dialog as one JSON object per dialog panel. A new mesh starts from the Mesh " +
                "tool defaults, or from FavoriteName, and the given settings override them. Create updates the " +
                "meshes this component created before (kept in the .gh file) instead of creating new ones; a mesh " +
                "deleted since then is reported in Errors, not created again. Switch this off or forget the " +
                "meshes in the component menu. Holes, arcs and level lines are kept as they are; the " +
                "ModifyMeshes component writes them.")
        {
        }

        protected override int GeometryInputCount => 1;

        protected override void AddGeometryInputs()
        {
            inManager.AddCurveParameter(
                "Polygon",
                "Polygon",
                "The outline of each new mesh, a closed polyline of at least 3 vertices (Create; the Z of each " +
                "vertex is its height above the Mesh Reference Plane, set by Offset to Home Story)."
                    .WithTypeName("curveList"),
                GH_ParamAccess.list);
        }

        protected override void AddGeometryOutputs()
        {
            OutCurves(
                "Polygon",
                "The outline of each mesh, a closed polyline; the Z of each vertex is its height above the " +
                "Mesh Reference Plane. Arcs of the outline are given as straight segments.");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = new List<JObject>();
            var curves = new List<Curve>();
            da.GetDataList(firstInputIndex, curves);
            for (var i = 0; i < curves.Count; i++)
            {
                if (curves[i] == null ||
                    !curves[i].TryGetPolyline(out Polyline polyline))
                {
                    this.AddError($"Polygon item {i} is not a polyline.");
                    return false;
                }

                var points = polyline.ToList();
                if (points.Count > 1 && points[0].DistanceTo(points[points.Count - 1]) < 1e-9)
                {
                    points.RemoveAt(points.Count - 1);
                }
                if (points.Count < 3)
                {
                    this.AddError($"Polygon item {i} has fewer than 3 vertices.");
                    return false;
                }

                geometry.Add(new JObject
                {
                    ["polygonCoordinates"] = new JArray(
                        points.Select(p => new JObject { ["x"] = p.X, ["y"] = p.Y, ["z"] = p.Z }))
                });
            }

            return true;
        }

        protected override void SetGeometryOutputs(
            IGH_DataAccess da,
            int firstOutputIndex,
            List<JObject> details)
        {
            da.SetDataList(
                firstOutputIndex,
                details.Select(d => PolygonOf(d)));
        }

        private static Curve PolygonOf(
            JObject details)
        {
            if (!(details?["polygonCoordinates"] is JArray coordinates) ||
                coordinates.Count < 3)
            {
                return null;
            }

            var points = coordinates
                .Select(
                    c => new Point3d(
                        (double) c["x"],
                        (double) c["y"],
                        (double?) c["z"] ?? 0.0))
                .ToList();
            if (points[0].DistanceTo(points[points.Count - 1]) > 1e-9)
            {
                points.Add(points[0]);
            }
            return new PolylineCurve(points);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMeshes;

        public override Guid ComponentGuid =>
            new Guid("7c3e5a92-41d8-4f6b-9e07-b2d5c1a8f364");
    }
}
