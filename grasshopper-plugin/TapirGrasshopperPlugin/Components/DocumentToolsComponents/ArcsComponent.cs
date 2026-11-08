using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Components.DesignToolsComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DocumentToolsComponents
{
    // The Arc/Circle tool: reads and creates arcs with the settings of the
    // tool's settings dialog, from ToolSettings/Arc.json. Origin, Radius,
    // BegAngle and EndAngle define a new arc. There is no ModifyArcs command,
    // so Execute only writes the Element ID, the layer and the draw order of
    // existing arcs.
    public class ArcsComponent : ElementToolComponent
    {
        protected override string ToolName => "Arc";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateArcs";
        protected override string CreateArrayKey => "arcsData";
        protected override string ModifyCommand => null;
        protected override string ModifyArrayKey => null;

        // The item properties of the CreateArcs input schema
        // (ElementCreationCommands.cpp).
        private static readonly ISet<string> CreateArcsKeys = new HashSet<string>
        {
            "begAngle", "endAngle", "favoriteName", "floorInd", "id", "layerIndex", "linePenIndex",
            "lineTypeId", "origin", "radius", "roomSeparator"
        };

        protected override ISet<string> CreateKeys => CreateArcsKeys;
        protected override ISet<string> ModifyKeys { get; } = new HashSet<string>();

        // The panels of the Arc/Circle tool's settings dialog in the table, in
        // its order; append-only (see ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "General Settings"
        };

        public ArcsComponent()
            : base(
                "Arcs",
                "Reads (Refresh) and creates (Create) arcs, with the settings of the Arc/Circle tool's settings " +
                "dialog as one JSON object per dialog panel. A new arc starts from the Arc/Circle tool defaults, or from " +
                "FavoriteName, and the given settings override them. Angles are in radians, counterclockwise from " +
                "the X axis. The story of a new arc can be given as {\"floorIndex\": ...} in any JSON input. " +
                "Archicad has no command to modify arcs, so Execute only writes ElementID, LayerIndex and DrawOrder.",
                GroupNames.DocumentTools)
        {
        }

        protected override int GeometryInputCount => 4;

        protected override void AddGeometryInputs()
        {
            InPoints(
                "Origin",
                "The center point of each new arc (Create; only X and Y are used).");
            InNumbers(
                "Radius",
                "The radius of each new arc (Create): one for all or one per origin.");
            InNumbers(
                "BegAngle",
                "The beginning angle of each new arc, in radians (Create): one for all or one per origin.");
            InNumbers(
                "EndAngle",
                "The end angle of each new arc, in radians (Create): one for all or one per origin.");
        }

        protected override void AddGeometryOutputs()
        {
            OutPoints(
                "Origin",
                "The center point of each arc, at the level of its home story.");
            OutNumberList(
                "Radius",
                "The radius of each arc.");
            OutNumberList(
                "BegAngle",
                "The beginning angle of each arc, in radians.");
            OutNumberList(
                "EndAngle",
                "The end angle of each arc, in radians.");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = null;
            var origins = new List<Point3d>();
            da.GetDataList(firstInputIndex, origins);
            var radii = da.OptionalList<double>(firstInputIndex + 1);
            var begAngles = da.OptionalList<double>(firstInputIndex + 2);
            var endAngles = da.OptionalList<double>(firstInputIndex + 3);

            if (!this.CheckSizes(
                    origins.Count,
                    "Origin",
                    ("Radius", radii.Count),
                    ("BegAngle", begAngles.Count),
                    ("EndAngle", endAngles.Count)))
            {
                return false;
            }

            geometry = new List<JObject>();
            if (origins.Count == 0)
            {
                return true;
            }

            if (radii.Count == 0 || begAngles.Count == 0 || endAngles.Count == 0)
            {
                this.AddError("Create needs Radius, BegAngle and EndAngle for the arcs.");
                return false;
            }

            for (var i = 0; i < origins.Count; i++)
            {
                geometry.Add(new JObject
                {
                    ["origin"] = new JObject { ["x"] = origins[i].X, ["y"] = origins[i].Y },
                    ["radius"] = radii.At(i),
                    ["begAngle"] = begAngles.At(i),
                    ["endAngle"] = endAngles.At(i)
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
                details.Select(d => OriginOf(d)));
            da.SetDataList(
                firstOutputIndex + 1,
                details.Select(d => (double?) d?["radius"]));
            da.SetDataList(
                firstOutputIndex + 2,
                details.Select(d => (double?) d?["begAngle"]));
            da.SetDataList(
                firstOutputIndex + 3,
                details.Select(d => (double?) d?["endAngle"]));
        }

        private static object OriginOf(
            JObject details)
        {
            var origin = details?["origin"];
            if (origin == null || origin.Type == JTokenType.Null)
            {
                return null;
            }

            return new Point3d(
                (double?) origin["x"] ?? 0.0,
                (double?) origin["y"] ?? 0.0,
                (double?) details["zCoordinate"] ?? 0.0);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateArcs;

        public override Guid ComponentGuid =>
            new Guid("b6e3f2a4-9c1d-4e57-8a0b-3d7f5c21e984");
    }
}
