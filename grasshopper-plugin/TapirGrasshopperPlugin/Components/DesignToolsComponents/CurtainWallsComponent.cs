using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Curtain Wall tool: reads, writes and creates curtain walls with the
    // settings of the Curtain Wall Settings dialog, from
    // ToolSettings/CurtainWall.json. ReferenceLine defines a new curtain wall
    // (a polyline; only X and Y are used; each pair of consecutive vertices is
    // one segment). CreateCurtainWalls and ModifyCurtainWalls take the same
    // fields (ModifyCurtainWalls keeps the number of segments), so Create can
    // update the curtain walls it made before.
    public class CurtainWallsComponent : ElementToolComponent
    {
        protected override string ToolName => "CurtainWall";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateCurtainWalls";
        protected override string CreateArrayKey => "curtainWallsData";
        protected override string ModifyCommand => "ModifyCurtainWalls";
        protected override string ModifyArrayKey => "curtainWallsWithDetails";

        // The item properties of the CreateCurtainWalls input schema
        // (ElementCreationCommands.cpp) and of the ModifyCurtainWalls one
        // (ExtendedElementCommands.cpp).
        private static readonly ISet<string> CreateCurtainWallsKeys = new HashSet<string>
        {
            "cells", "coordinates", "favoriteName", "floorIndex", "height", "id", "primaryPattern",
            "secondaryPattern", "zCoordinate"
        };

        private static readonly ISet<string> ModifyCurtainWallsKeys = new HashSet<string>
        {
            "cells", "coordinates", "floorIndex", "height", "primaryPattern", "secondaryPattern", "zCoordinate"
        };

        // ModifyCurtainWalls takes the reference line (with the same number of
        // vertices), so Create updates the curtain walls this component made
        // before instead of making new ones.
        protected override bool CanUpdateCreatedElements => true;

        protected override ISet<string> CreateKeys => CreateCurtainWallsKeys;
        protected override ISet<string> ModifyKeys => ModifyCurtainWallsKeys;

        // The Curtain Wall Settings dialog's panels, in the order of the table;
        // append-only (see ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Geometry and Positioning",
            "Grid Pattern and Preview",
            "Floor Plan and Section"
        };

        public CurtainWallsComponent()
            : base(
                "CurtainWalls",
                "Reads (Refresh), writes (Execute) and creates (Create) curtain walls, with the settings of the " +
                "Curtain Wall Settings dialog as one JSON object per dialog panel. A new curtain wall starts from " +
                "the Curtain Wall tool defaults, or from FavoriteName, and the given settings override them; the " +
                "frame and panel classes come from there. The column and row patterns (Columns, Rows) and the " +
                "cells are written as in CreateCurtainWalls, but not read back. Create updates the curtain walls " +
                "this component created before (kept in the .gh file) instead of creating new ones; their " +
                "reference line can move, but keeps its number of vertices, and a curtain wall deleted since " +
                "then is reported in Errors, not created again. Switch this off or forget the curtain walls in " +
                "the component menu.")
        {
        }

        protected override int GeometryInputCount => 1;

        protected override void AddGeometryInputs()
        {
            inManager.AddCurveParameter(
                "ReferenceLine",
                "ReferenceLine",
                "The reference line of each new curtain wall, a polyline (Create; only X and Y are used; each " +
                "pair of consecutive vertices becomes one straight segment)."
                    .WithTypeName("curveList"),
                GH_ParamAccess.list);
        }

        protected override void AddGeometryOutputs()
        {
            OutCurves(
                "ReferenceLine",
                "The reference line of each curtain wall, at the elevation of its bottom.");
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
                    !curves[i].TryGetPolyline(out Polyline polyline) ||
                    polyline.Count < 2)
                {
                    this.AddError($"ReferenceLine item {i} is not a polyline of at least 2 vertices.");
                    return false;
                }

                geometry.Add(new JObject
                {
                    ["coordinates"] = new JArray(
                        polyline.Select(p => new JObject { ["x"] = p.X, ["y"] = p.Y }))
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
                details.Select(d => ReferenceLineOf(d)));
        }

        private static Curve ReferenceLineOf(
            JObject details)
        {
            if (!(details?["referenceLine"] is JArray coordinates) ||
                coordinates.Count < 2)
            {
                return null;
            }

            var z = (double?) details["zCoordinate"] ?? 0.0;
            return new PolylineCurve(
                coordinates.Select(
                    c => new Point3d(
                        (double) c["x"],
                        (double) c["y"],
                        z)));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateCurtainWalls;

        public override Guid ComponentGuid =>
            new Guid("6b0e2f4a-91c3-4d7e-8a25-c3f7d19e5b62");
    }
}
