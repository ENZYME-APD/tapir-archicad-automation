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
    // The Circle tool (the Arc/Circle tool's circles): reads and creates
    // circles with the settings of its settings dialog, from
    // ToolSettings/Circle.json. Origin and Radius define a new circle. There
    // is no ModifyCircles command: Execute writes the General Settings with
    // SetDetailsOfElements (typeSpecificDetails, ArcSettings), and the
    // Element ID, the layer, the draw order and the story.
    public class CirclesComponent : ElementToolComponent
    {
        protected override string ToolName => "Circle";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateCircles";
        protected override string CreateArrayKey => "circlesData";
        protected override string ModifyCommand => null;
        protected override string ModifyArrayKey => null;

        // The item properties of the CreateCircles input schema
        // (ElementCreationCommands.cpp, with LineFamilySettingsSchemaProperties).
        // The story is taken as "floorIndex" here and sent as "floorInd" (see
        // PrepareSettings and PrepareCreateItem).
        private static readonly ISet<string> CreateCirclesKeys = new HashSet<string>
        {
            "favoriteName", "floorIndex", "id", "layerIndex", "lineTypeId", "linePenIndex", "origin", "radius",
            "roomSeparator"
        };

        protected override ISet<string> CreateKeys => CreateCirclesKeys;
        protected override ISet<string> ModifyKeys { get; } = new HashSet<string>();

        // What SetDetailsOfElements writes of a circle (its ArcSettings);
        // "radius" is a key only the arc settings have.
        protected override ISet<string> TypeSpecificDetailsKeys { get; } = new HashSet<string>
        {
            "linePenIndex", "lineTypeId", "origin", "radius", "roomSeparator"
        };

        protected override string DetailsTypeKey => "radius";

        // The settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "General Settings"
        };

        public CirclesComponent()
            : base(
                "Circles",
                "Reads (Refresh), writes (Execute) and creates (Create) circles, with the settings of the circle's " +
                "settings dialog as one JSON object per dialog panel. A new circle starts from the tool defaults, " +
                "or from FavoriteName, and the given settings override them. The story can be given as " +
                "{\"floorIndex\": ...} in any JSON input. Origin and Radius are only used by Create.",
                GroupNames.DocumentTools)
        {
        }

        // Create takes the story as "floorInd", SetDetailsOfElements (Execute)
        // as "floorIndex"; the table's set name is "floorInd".
        protected override void PrepareSettings(
            JObject settings,
            ISet<string> notes)
        {
            var floor = settings["floorInd"];
            if (floor != null)
            {
                settings.Remove("floorInd");
                settings["floorIndex"] = floor;
            }
        }

        protected override void PrepareCreateItem(
            JObject item)
        {
            var floor = item["floorIndex"];
            if (floor != null)
            {
                item.Remove("floorIndex");
                item["floorInd"] = floor;
            }
        }

        protected override int GeometryInputCount => 2;

        protected override void AddGeometryInputs()
        {
            InPoints(
                "Origin",
                "The center point of each new circle (Create; only X and Y are used).");
            InNumbers(
                "Radius",
                "The radius of each new circle (Create): one for all or one per origin.");
        }

        protected override void AddGeometryOutputs()
        {
            OutPoints(
                "Origin",
                "The center point of each circle (Z is the level of its home story).");
            OutNumberList(
                "Radius",
                "The radius of each circle.");
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

            if (origins.Count > 0 && radii.Count == 0)
            {
                this.AddError("Create needs the Radius of the new circles.");
                return false;
            }
            if (!this.CheckSizes(
                    origins.Count,
                    "Origin",
                    ("Radius", radii.Count)))
            {
                return false;
            }

            geometry = new List<JObject>();
            for (var i = 0; i < origins.Count; i++)
            {
                geometry.Add(
                    new JObject
                    {
                        ["origin"] = new JObject { ["x"] = origins[i].X, ["y"] = origins[i].Y },
                        ["radius"] = radii.At(i)
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
                origin.Value<double?>("x") ?? 0.0,
                origin.Value<double?>("y") ?? 0.0,
                details.Value<double?>("zCoordinate") ?? 0.0);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateCircles;

        public override Guid ComponentGuid =>
            new Guid("4b8e2c71-9d3a-4f65-b1e0-6c2a7d58f913");
    }
}
