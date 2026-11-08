using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Components.DesignToolsComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DocumentToolsComponents
{
    // The Fill tool: reads and creates fills (hatches) with the settings of
    // the Fill Settings dialog, from ToolSettings/Fill.json. Outline and Holes
    // define a new fill. There is no ModifyHatches command, so Execute only
    // writes the Element ID, the layer and the draw order of existing fills.
    public class FillsComponent : ElementToolComponent
    {
        protected override string ToolName => "Fill";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateHatches";
        protected override string CreateArrayKey => "hatchesData";
        protected override string ModifyCommand => null;
        protected override string ModifyArrayKey => null;

        // The item properties of the CreateHatches input schema
        // (ElementCreationCommands.cpp).
        private static readonly ISet<string> CreateHatchesKeys = new HashSet<string>
        {
            "arcs", "backgroundColor", "buildingMaterialId", "contourPenIndex", "coordinates", "favoriteName",
            "fillBackgroundPenIndex", "fillId", "fillPenIndex", "floorInd", "foregroundColor", "holes", "id",
            "layerIndex", "roomSpecial", "showArea"
        };

        protected override ISet<string> CreateKeys => CreateHatchesKeys;
        protected override ISet<string> ModifyKeys { get; } = new HashSet<string>();

        // The Fill Settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "General Settings"
        };

        public FillsComponent()
            : base(
                "Fills",
                "Reads (Refresh) and creates (Create) fills, with the settings of the Fill Settings dialog as " +
                "one JSON object per dialog panel. A new fill starts from the Fill tool defaults, or from " +
                "FavoriteName, and the given settings override them. A given fillId makes it a fill of that " +
                "fill type, a given buildingMaterialId a building material fill; foregroundColor and " +
                "backgroundColor override the pens. The story can be given as {\"floorIndex\": ...} in any JSON " +
                "input. Archicad has no command to modify fills, so Execute only writes ElementID, LayerIndex " +
                "and DrawOrder.",
                GroupNames.DocumentTools)
        {
        }

        protected override int GeometryInputCount => 2;

        protected override void AddGeometryInputs()
        {
            inManager.AddCurveParameter(
                "Outline",
                "Outline",
                ("The outline of each new fill, a closed curve (Create; only X and Y are used; line and arc " +
                 "segments are kept, other curves are approximated with a polyline).")
                    .WithTypeName("curveList"),
                GH_ParamAccess.list);
            inManager.AddCurveParameter(
                "Holes",
                "Holes",
                ("The closed curves of the holes of each new fill (Create): one branch per Outline curve, an " +
                 "empty branch for a fill without holes. Optional.")
                    .WithTypeName("curveTree"),
                GH_ParamAccess.tree);
        }

        protected override void AddGeometryOutputs()
        {
            OutCurves(
                "Outline",
                "The outline of each fill, at the elevation of its story.");
            OutCurveTree(
                "Holes",
                "The holes of each fill, one branch per fill.");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = null;
            var outlines = new List<Curve>();
            da.GetDataList(firstInputIndex, outlines);
            da.GetDataTree(firstInputIndex + 1, out GH_Structure<GH_Curve> holeTree);

            var hasHoles = holeTree != null && holeTree.DataCount > 0;
            if (hasHoles && holeTree.Branches.Count != outlines.Count)
            {
                this.AddError(
                    $"Holes has {holeTree.Branches.Count} branches: give one branch per Outline curve ({outlines.Count}).");
                return false;
            }

            var items = new List<JObject>();
            for (var i = 0; i < outlines.Count; i++)
            {
                if (!PolygonHelp.TryToCoordinates(
                        outlines[i],
                        true,
                        out var coordinates,
                        out var arcs,
                        out var error))
                {
                    this.AddError($"Outline item {i}: {error}.");
                    return false;
                }

                var item = new JObject
                {
                    ["coordinates"] = coordinates,
                    ["arcs"] = arcs
                };

                var holes = new JArray();
                if (hasHoles)
                {
                    foreach (var holeCurve in holeTree.Branches[i])
                    {
                        if (holeCurve?.Value == null)
                        {
                            continue;
                        }

                        if (!PolygonHelp.TryToCoordinates(
                                holeCurve.Value,
                                true,
                                out var holeCoordinates,
                                out var holeArcs,
                                out var holeError))
                        {
                            this.AddError($"Holes of Outline item {i}: {holeError}.");
                            return false;
                        }

                        var hole = new JObject { ["polygonOutline"] = holeCoordinates };
                        if (holeArcs.Count > 0)
                        {
                            hole["polygonArcs"] = holeArcs;
                        }
                        holes.Add(hole);
                    }
                }

                // Always given, so holes in a JSON read from another fill
                // never end up in the new one.
                item["holes"] = holes;
                items.Add(item);
            }

            geometry = items;
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

                var z = (double?) details[i]["zCoordinate"] ?? 0.0;
                foreach (var hole in holeArray)
                {
                    var curve = PolygonHelp.ToCurve(
                        hole["polygonOutline"] ?? hole["polygonCoordinates"],
                        hole["polygonArcs"],
                        true,
                        z);
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
            if (details == null)
            {
                return null;
            }

            return PolygonHelp.ToCurve(
                details["coordinates"],
                details["arcs"],
                true,
                (double?) details["zCoordinate"] ?? 0.0);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateHatches;

        public override Guid ComponentGuid =>
            new Guid("5b0e8c47-2f6a-4d39-9e1b-7c3a8f24d6e1");
    }
}
