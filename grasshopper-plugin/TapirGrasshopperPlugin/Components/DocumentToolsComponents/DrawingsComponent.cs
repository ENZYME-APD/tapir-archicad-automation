using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Components.DesignToolsComponents;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.DocumentToolsComponents
{
    // The Drawing tool: reads and places drawings with the settings of the
    // Drawing Settings dialog, from ToolSettings/Drawing.json. NavigatorItemGuid,
    // LayoutDatabaseGuid, Position, ClipPolygon and ModelOffset define a new
    // drawing. There is no ModifyDrawings command, so Execute only writes the
    // Element ID, the layer and the draw order of existing drawings.
    public class DrawingsComponent : ElementToolComponent
    {
        protected override string ToolName => "Drawing";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateDrawings";
        protected override string CreateArrayKey => "drawingsData";
        protected override string ModifyCommand => null;
        protected override string ModifyArrayKey => null;

        // The item properties of the CreateDrawings input schema
        // (DocumentCreationCommands.cpp). It takes no favoriteName.
        private static readonly ISet<string> CreateDrawingsKeys = new HashSet<string>
        {
            "angle", "clipPolygon", "drawingScale", "layoutDatabaseId", "modelOffset", "name", "nameType",
            "navigatorItemId", "position", "scale"
        };

        protected override ISet<string> CreateKeys => CreateDrawingsKeys;
        protected override ISet<string> ModifyKeys { get; } = new HashSet<string>();

        // The Drawing Settings dialog's panels, in the order of the table;
        // append-only (see ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Identification",
            "Size and Appearance",
            "Frame",
            "Title"
        };

        private bool _favoriteNameIgnored;

        public DrawingsComponent()
            : base(
                "Drawings",
                "Reads (Refresh) and places (Create) drawings, with the settings of the Drawing Settings dialog " +
                "as one JSON object per dialog panel. Create places one drawing per NavigatorItemGuid (a view or " +
                "viewpoint) on the layout of LayoutDatabaseGuid, or on the active layout, starting from the " +
                "Drawing tool defaults; the given settings override them. Archicad has no command to modify " +
                "drawings, so Execute only writes ElementID, LayerIndex and DrawOrder.",
                GroupNames.DocumentTools)
        {
        }

        protected override int GeometryInputCount => 5;

        protected override void AddGeometryInputs()
        {
            InGenerics(
                "NavigatorItemGuid",
                "The view or viewpoint each new drawing shows (Create): one drawing per item.");
            InGenerics(
                "LayoutDatabaseGuid",
                "The layout database each new drawing is placed on (Create): none (the active layout), one for " +
                "all or one per drawing.");
            InPoints(
                "Position",
                "Where each new drawing is placed on the layout (Create; only X and Y are used): one for all or " +
                "one per drawing. Without ClipPolygon it is the drawing's lower left corner, with ClipPolygon " +
                "the point where the origin of the source view lands.");
            inManager.AddCurveParameter(
                "ClipPolygon",
                "ClipPolygon",
                ("The polygon each new drawing is clipped with, a closed polyline in layout coordinates like " +
                 "Position (Create; only X and Y are used): none (not clipped), one for all or one per drawing.")
                    .WithTypeName("curveList"),
                GH_ParamAccess.list);
            InPoints(
                "ModelOffset",
                "The offset of the model origin within each new drawing, in model meters (Create; only X and Y " +
                "are used; Archicad uses it only with ClipPolygon): none, one for all or one per drawing.");
        }

        protected override void AddGeometryOutputs()
        {
            OutPoints(
                "Position",
                "The position (origin) of each drawing on its layout (Z is 0).");
            OutCurves(
                "ClipPolygon",
                "The clip polygon of each drawing on its layout (null when the drawing is not clipped).");
            OutPoints(
                "ModelOffset",
                "The offset of the model origin within each drawing (Z is 0).");
            OutCurves(
                "Bounds",
                "The bounding rectangle of each drawing on its layout.");
            OutGenerics(
                "TitleElementGuid",
                "The placed title object of each drawing (null when it has no title), e.g. for " +
                "the GDL parameter components.");
        }

        protected override void BeforeSolveInstance()
        {
            base.BeforeSolveInstance();
            _favoriteNameIgnored = false;
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = null;
            var navigatorWrappers = da.OptionalList<GH_ObjectWrapper>(firstInputIndex);
            var layoutWrappers = da.OptionalList<GH_ObjectWrapper>(firstInputIndex + 1);
            var positions = da.OptionalList<Point3d>(firstInputIndex + 2);
            var clipCurves = da.OptionalList<Curve>(firstInputIndex + 3);
            var modelOffsets = da.OptionalList<Point3d>(firstInputIndex + 4);

            var count = navigatorWrappers.Count;
            if (count > 0 && positions.Count == 0)
            {
                this.AddError("Create needs the Position of the drawings.");
                return false;
            }

            if (!this.CheckSizes(
                    count,
                    "NavigatorItemGuid",
                    ("LayoutDatabaseGuid", layoutWrappers.Count),
                    ("Position", positions.Count),
                    ("ClipPolygon", clipCurves.Count),
                    ("ModelOffset", modelOffsets.Count)))
            {
                return false;
            }

            var navigatorIds = new List<string>();
            foreach (var wrapper in navigatorWrappers)
            {
                var id = GuidObject<NavigatorGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid navigator item identifier in the NavigatorItemGuid input.");
                    return false;
                }
                navigatorIds.Add(id.Guid);
            }

            var layoutIds = new List<string>();
            foreach (var wrapper in layoutWrappers)
            {
                var id = GuidObject<DatabaseGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid database identifier in the LayoutDatabaseGuid input.");
                    return false;
                }
                layoutIds.Add(id.Guid);
            }

            var clipPolygons = new List<JArray>();
            for (var k = 0; k < clipCurves.Count; k++)
            {
                if (!TryGetClipPolygon(clipCurves[k], out var polygon))
                {
                    this.AddError($"ClipPolygon item {k} is not a polyline of at least 3 vertices.");
                    return false;
                }
                clipPolygons.Add(polygon);
            }

            geometry = new List<JObject>();
            for (var i = 0; i < count; i++)
            {
                var position = positions.At(i);
                var item = new JObject
                {
                    ["navigatorItemId"] = new JObject { ["guid"] = navigatorIds[i] },
                    ["position"] = new JObject { ["x"] = position.X, ["y"] = position.Y }
                };
                if (layoutIds.Count > 0)
                {
                    item["layoutDatabaseId"] = new JObject { ["guid"] = layoutIds.At(i) };
                }
                if (clipPolygons.Count > 0)
                {
                    item["clipPolygon"] = clipPolygons.At(i).DeepClone();
                }
                if (modelOffsets.Count > 0)
                {
                    var modelOffset = modelOffsets.At(i);
                    item["modelOffset"] = new JObject { ["x"] = modelOffset.X, ["y"] = modelOffset.Y };
                }
                geometry.Add(item);
            }

            return true;
        }

        // The vertices of a clip polygon, without the closing one.
        private static bool TryGetClipPolygon(
            Curve curve,
            out JArray polygon)
        {
            polygon = null;
            if (curve == null ||
                !curve.TryGetPolyline(out Polyline polyline))
            {
                return false;
            }

            var points = polyline.ToList();
            if (points.Count > 1 &&
                points[0].DistanceTo(points[points.Count - 1]) <= Rhino.RhinoMath.ZeroTolerance)
            {
                points.RemoveAt(points.Count - 1);
            }
            if (points.Count < 3)
            {
                return false;
            }

            polygon = new JArray(points.Select(p => new JObject { ["x"] = p.X, ["y"] = p.Y }));
            return true;
        }

        // CreateDrawings takes no favorite: a FavoriteName is dropped so the
        // command does not reject the whole request.
        protected override void PrepareCreateItem(
            JObject item)
        {
            if (item.Remove("favoriteName") && !_favoriteNameIgnored)
            {
                _favoriteNameIgnored = true;
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Drawings cannot be created from a favorite; FavoriteName was ignored.");
            }
        }

        protected override void SetGeometryOutputs(
            IGH_DataAccess da,
            int firstOutputIndex,
            List<JObject> details)
        {
            da.SetDataList(
                firstOutputIndex,
                details.Select(d => JsonOutputHelp.Point(d?["pos"])));
            da.SetDataList(
                firstOutputIndex + 1,
                details.Select(d => ClosedPolyline(d?["clipPolygon"] as JArray)));
            da.SetDataList(
                firstOutputIndex + 2,
                details.Select(d => JsonOutputHelp.Point(d?["modelOffset"])));
            da.SetDataList(
                firstOutputIndex + 3,
                details.Select(d => BoundsOf(d?["bounds"])));
            da.SetDataList(
                firstOutputIndex + 4,
                details.Select(d => ItemListHelp.ElementGuidOf(d?["titleElementId"])));
        }

        private static Curve ClosedPolyline(
            JArray coordinates)
        {
            if (coordinates == null ||
                coordinates.Count < 3)
            {
                return null;
            }

            var points = coordinates
                .Select(c => new Point3d(c.Value<double?>("x") ?? 0.0, c.Value<double?>("y") ?? 0.0, 0.0))
                .ToList();
            if (points[0].DistanceTo(points[points.Count - 1]) > Rhino.RhinoMath.ZeroTolerance)
            {
                points.Add(points[0]);
            }
            return new PolylineCurve(points);
        }

        private static Curve BoundsOf(
            JToken bounds)
        {
            if (bounds == null ||
                bounds.Type != JTokenType.Object)
            {
                return null;
            }

            var xMin = bounds.Value<double?>("xMin") ?? 0.0;
            var yMin = bounds.Value<double?>("yMin") ?? 0.0;
            var xMax = bounds.Value<double?>("xMax") ?? 0.0;
            var yMax = bounds.Value<double?>("yMax") ?? 0.0;
            return new PolylineCurve(
                new[]
                {
                    new Point3d(xMin, yMin, 0.0),
                    new Point3d(xMax, yMin, 0.0),
                    new Point3d(xMax, yMax, 0.0),
                    new Point3d(xMin, yMax, 0.0),
                    new Point3d(xMin, yMin, 0.0)
                });
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateDrawings;

        public override Guid ComponentGuid =>
            new Guid("5b7e2c41-9d3a-4f6e-8a12-c4e0d7b93f58");
    }
}
