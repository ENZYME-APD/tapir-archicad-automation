using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Railing tool: reads and creates railings with the settings of the
    // Railing Settings dialog, from ToolSettings/Railing.json. ReferenceLine
    // defines a new railing (a polyline; only X and Y are used). There is no
    // ModifyRailings command, so Execute only writes the Element ID, the layer
    // and the draw order of existing railings; the other settings can only be
    // given on Create.
    public class RailingsComponent : ElementToolComponent
    {
        protected override string ToolName => "Railing";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateRailings";
        protected override string CreateArrayKey => "railingsData";
        protected override string ModifyCommand => null;
        protected override string ModifyArrayKey => null;

        // The item properties of the CreateRailings input schema
        // (ElementCreationCommands.cpp).
        private static readonly ISet<string> CreateRailingsKeys = new HashSet<string>
        {
            "bottomOffset", "coordinates", "favoriteName", "floorIndex", "height", "id"
        };

        protected override ISet<string> CreateKeys => CreateRailingsKeys;
        protected override ISet<string> ModifyKeys { get; } = new HashSet<string>();

        // The Railing Settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Geometry and Positioning",
            "Segment Settings"
        };

        public RailingsComponent()
            : base(
                "Railings",
                "Reads (Refresh) and creates (Create) railings, with the settings of the Railing Settings dialog " +
                "as one JSON object per dialog panel. A new railing starts from the Railing tool defaults, or " +
                "from FavoriteName, and the given settings override them; Archicad generates one node per vertex " +
                "and one segment per pair of consecutive vertices of the reference line, with the posts, rails, " +
                "handrails, toprails, panels and balusters of the favorite or the defaults. Archicad has no " +
                "command to modify railings, so Execute only writes ElementID, LayerIndex and DrawOrder.")
        {
        }

        protected override int GeometryInputCount => 1;

        protected override void AddGeometryInputs()
        {
            inManager.AddCurveParameter(
                "ReferenceLine",
                "ReferenceLine",
                "The reference line of each new railing, a polyline (Create; only X and Y are used; each pair " +
                "of consecutive vertices becomes one straight segment)."
                    .WithTypeName("curveList"),
                GH_ParamAccess.list);
        }

        protected override void AddGeometryOutputs()
        {
            OutCurves(
                "ReferenceLine",
                "The reference line of each railing, at the elevation of its base.");
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
            Properties.Resources.CreateRailings;

        public override Guid ComponentGuid =>
            new Guid("21cbe330-8c0d-4a94-a165-ba7ac22dad00");
    }
}
