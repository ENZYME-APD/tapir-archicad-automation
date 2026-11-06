using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Slab tool: reads, writes and creates slabs with the settings of the
    // Slab Settings dialog, from ToolSettings/Slab.json. Outline and Holes
    // define a new slab: X and Y of the curves give its polygon, and the Z of
    // the outline is the elevation of its reference plane (zCoordinate, as
    // GetDetailsOfElements returns it), so the read curves can be fed back.
    public class SlabsComponent : ElementToolComponent
    {
        protected override string ToolName => "Slab";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateSlabs";
        protected override string CreateArrayKey => "slabsData";
        protected override string ModifyCommand => "ModifySlabs";
        protected override string ModifyArrayKey => "slabsWithDetails";

        // The item properties of the CreateSlabs input schema
        // (ElementCreationCommands.cpp), plus zCoordinate and polygonOutline,
        // the names ModifySlabs and GetDetailsOfElements use for its level
        // and polygonCoordinates: PrepareCreateItem renames them.
        private static readonly ISet<string> CreateSlabsKeys = new HashSet<string>
        {
            "buildingMaterialId", "compositeId", "favoriteName", "floorIndex", "holes", "id", "layerIndex",
            "level", "polygonArcs", "polygonCoordinates", "referencePlaneLocation", "structureType", "thickness",
            "zCoordinate", "polygonOutline"
        };

        // The item properties of the ModifySlabs input schema
        // (ExtendedElementCommands.cpp).
        private static readonly ISet<string> ModifySlabsKeys = new HashSet<string>
        {
            "bottomMaterial", "buildingMaterialId", "compositeId", "cutFillBackgroundPen", "cutFillPen",
            "floorFill", "floorIndex", "holes", "polygonArcs", "polygonOutline", "referencePlaneLocation",
            "sideMaterial", "structureType", "thickness", "topMaterial", "zCoordinate"
        };

        // ModifySlabs takes the outline, the holes and the elevation, so
        // Create updates the slabs this component made before instead of
        // making new ones.
        protected override bool CanUpdateCreatedElements => true;

        protected override ISet<string> CreateKeys => CreateSlabsKeys;
        protected override ISet<string> ModifyKeys => ModifySlabsKeys;

        // The Slab Settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Geometry and Positioning",
            "Floor Plan and Section",
            "Model"
        };

        public SlabsComponent()
            : base(
                "Slabs",
                "Reads (Refresh), writes (Execute) and creates (Create) slabs, with the settings of the " +
                "Slab Settings dialog as one JSON object per dialog panel. A new slab starts from the Slab " +
                "tool defaults, or from FavoriteName, and the given settings override them. Create updates the " +
                "slabs this component created before (kept in the .gh file) instead of creating new ones; a slab " +
                "deleted since then is reported in Errors, not created again. Switch this off or forget the " +
                "slabs in the component menu.")
        {
        }

        protected override int GeometryInputCount => 2;

        protected override void AddGeometryInputs()
        {
            inManager.AddCurveParameter(
                "Outline",
                "Outline",
                ("The closed outline curve of each new slab (Create). Line and arc segments are kept as they are; " +
                 "anything else is approximated with a polyline. X and Y give the polygon, the Z of the curve's " +
                 "start is the elevation of the slab's reference plane.")
                    .WithTypeName("curveList"),
                GH_ParamAccess.list);
            inManager.AddCurveParameter(
                "Holes",
                "Holes",
                ("The closed curves of the holes of each new slab (Create; only X and Y are used): one branch per " +
                 "Outline curve, in its order, an empty branch for a slab without holes. Without Holes the slabs " +
                 "have none.")
                    .WithTypeName("curveTree"),
                GH_ParamAccess.tree);
        }

        protected override void AddGeometryOutputs()
        {
            OutCurves(
                "Outline",
                "The outline of each slab, at the elevation of its reference plane.");
            OutCurveTree(
                "Holes",
                "The holes of each slab, at the elevation of its reference plane: one branch per slab.");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = new List<JObject>();
            var outlines = new List<Curve>();
            da.GetDataList(firstInputIndex, outlines);

            var holeBranches = new List<List<GH_Curve>>();
            if (da.TryGetTree(firstInputIndex + 1, out GH_Structure<GH_Curve> holeTree))
            {
                holeBranches = holeTree.Branches.ToList();
                // An unconnected or empty input means no holes.
                if (holeBranches.All(b => b.Count == 0 || b.All(c => c == null)))
                {
                    holeBranches = new List<List<GH_Curve>>();
                }
            }
            if (holeBranches.Count > 0 && holeBranches.Count != outlines.Count)
            {
                this.AddError(
                    $"Holes has {holeBranches.Count} branches and Outline {outlines.Count} curves: give one branch " +
                    "per Outline curve (an empty branch for a slab without holes).");
                return false;
            }

            for (var i = 0; i < outlines.Count; i++)
            {
                var outline = outlines[i];
                if (!PolygonHelp.TryToCoordinates(outline, true, out var coordinates, out var arcs, out var error))
                {
                    this.AddError($"Outline item {i} is invalid: {error}.");
                    return false;
                }

                var item = new JObject
                {
                    ["polygonOutline"] = coordinates,
                    ["polygonArcs"] = arcs,
                    ["zCoordinate"] = outline.PointAtStart.Z
                };

                if (holeBranches.Count > 0)
                {
                    var holes = new JArray();
                    foreach (var ghCurve in holeBranches[i])
                    {
                        if (ghCurve?.Value == null)
                        {
                            continue;
                        }
                        if (!PolygonHelp.TryToCoordinates(ghCurve.Value, true, out var holeCoordinates, out var holeArcs, out error))
                        {
                            this.AddError($"A hole of Outline item {i} is invalid: {error}.");
                            return false;
                        }
                        var hole = new JObject { ["polygonOutline"] = holeCoordinates };
                        if (holeArcs.Count > 0)
                        {
                            hole["polygonArcs"] = holeArcs;
                        }
                        holes.Add(hole);
                    }
                    if (holes.Count > 0)
                    {
                        item["holes"] = holes;
                    }
                }

                geometry.Add(item);
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
                details.Select(d => OutlineOf(d)));

            var holes = new DataTree<Curve>();
            for (var i = 0; i < details.Count; i++)
            {
                var path = new GH_Path(da.Iteration, i);
                holes.EnsurePath(path);
                if (!(details[i]?["holes"] is JArray holeArray))
                {
                    continue;
                }
                var z = ZOf(details[i]);
                foreach (var hole in holeArray)
                {
                    // GetDetailsOfElements repeats the first coordinate at the
                    // end of each contour, so the polygon is closed already.
                    var curve = PolygonHelp.ToCurve(hole?["polygonOutline"], hole?["polygonArcs"], false, z);
                    if (curve != null)
                    {
                        holes.Add(curve, path);
                    }
                }
            }
            da.SetDataTree(firstOutputIndex + 1, holes);
        }

        private static Curve OutlineOf(
            JObject details)
        {
            return details == null
                ? null
                : PolygonHelp.ToCurve(details["polygonOutline"], details["polygonArcs"], false, ZOf(details));
        }

        private static double ZOf(
            JObject details)
        {
            return (double?) details?["zCoordinate"] ?? 0.0;
        }

        // GetDetailsOfElements returns the reference plane's elevation twice:
        // as zCoordinate (absolute, which ModifySlabs and CreateSlabs take) and
        // as level (from the home story, which neither takes), so level is
        // left out and zCoordinate is written.
        protected override void PrepareSettings(
            JObject settings,
            ISet<string> notes)
        {
            if (settings["level"] == null)
            {
                return;
            }
            if (settings["zCoordinate"] == null)
            {
                notes.Add(
                    $"{KeyOfField("level")} (from the home story) cannot be set; give " +
                    $"{KeyOfField("zCoordinate")} (the elevation of the reference plane) instead.");
            }
            settings.Remove("level");
        }

        // CreateSlabs calls the elevation level (an absolute Z, measured from
        // the home story's elevation when floorIndex is given) and the outline
        // polygonCoordinates.
        protected override void PrepareCreateItem(
            JObject item)
        {
            Rename(item, "zCoordinate", "level");
            Rename(item, "polygonOutline", "polygonCoordinates");
            if (item["polygonArcs"] is JArray arcs && arcs.Count == 0)
            {
                item.Remove("polygonArcs");
            }
        }

        private static void Rename(
            JObject item,
            string from,
            string to)
        {
            var value = item[from];
            if (value == null)
            {
                return;
            }
            item.Remove(from);
            item[to] = value;
        }

        private string KeyOfField(
            string apiName)
        {
            return Table.Find(apiName)?.Key(UseLabels) ?? apiName;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SlabDetails;

        public override Guid ComponentGuid =>
            new Guid("5a3e8f21-7c4d-4b9e-a6f2-3d81c0e5b947");
    }
}
