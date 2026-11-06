using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Beam tool: reads, writes and creates beams with the settings of the
    // Beam Settings dialog, from ToolSettings/Beam.json. ReferenceLine
    // defines a new beam: its start and end point in X and Y, and with the Z
    // of its start point the elevation of the beam's reference line
    // (CreateBeams' zCoordinate), as in the CreateBeams component.
    public class BeamsComponent : ElementToolComponent
    {
        protected override string ToolName => "Beam";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateBeams";
        protected override string CreateArrayKey => "beamsData";
        protected override string ModifyCommand => "ModifyBeams";
        protected override string ModifyArrayKey => "beamsWithDetails";

        // The item properties of the CreateBeams and ModifyBeams input schemas
        // (ExtendedElementCommands.cpp); a setting the Modify command does not
        // take is written by Create, and the other way round.
        private static readonly ISet<string> CreateBeamsKeys = new HashSet<string>
        {
            "anchorPoint", "arcAngle", "begCoordinate", "buildingMaterialId", "circleBased", "endCoordinate",
            "favoriteName", "floorIndex", "height", "id", "isSlanted", "isWidthAndHeightLinked", "layerIndex",
            "offset", "profileAngle", "profileId", "slantAngle", "verticalCurveHeight", "width", "zCoordinate"
        };

        private static readonly ISet<string> ModifyBeamsKeys = new HashSet<string>
        {
            "anchorPoint", "arcAngle", "beamShape", "begCoordinate", "buildingMaterialId", "circleBased",
            "coverFill", "cutFillBackgroundPen", "cutFillPen", "endCoordinate", "height", "holes", "isFlipped",
            "isSlanted", "isWidthAndHeightLinked", "level", "offset", "profileAngle", "profileId", "slantAngle",
            "verticalCurveHeight", "width"
        };

        protected override ISet<string> CreateKeys => CreateBeamsKeys;
        protected override ISet<string> ModifyKeys => ModifyBeamsKeys;

        // The panels of the Beam table, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Positioning",
            "End Cuts and Reference Axis",
            "Structure",
            "Floor Plan and Section"
        };

        public BeamsComponent()
            : base(
                "Beams",
                "Reads (Refresh), writes (Execute) and creates (Create) beams, with the settings of the " +
                "Beam Settings dialog as one JSON object per dialog panel. A new beam starts from the Beam " +
                "tool defaults, or from FavoriteName, and the given settings override them. ReferenceLine " +
                "places a new beam: the Z of its start point is the elevation of the beam's reference line, " +
                "on the home story it lies in unless Home Story is given.")
        {
        }

        // A horizontally curved beam needs its arc angle, which no panel of
        // the table holds, so a shape read by Refresh could not be written
        // back as it was: it is left unchanged instead.
        protected override void PrepareSettings(
            JObject settings,
            ISet<string> notes)
        {
            if (settings["beamShape"]?.ToString() == "HorizontallyCurved" && settings["arcAngle"] == null)
            {
                settings.Remove("beamShape");
                notes.Add("A horizontally curved beam shape cannot be set without its arc angle; it was left unchanged.");
            }
        }

        // On Create the Z of ReferenceLine sets the elevation of the new beam
        // (with Home Story, its offset to the home story), so an Offset to Home
        // Story read by Refresh from another beam must not move it afterwards.
        protected override void PrepareCreateSettings(
            JObject settings,
            ISet<string> notes)
        {
            if (settings.Remove("level"))
            {
                notes.Add(
                    $"{Table.Find("level")?.Key(UseLabels) ?? "level"} is not used on Create: " +
                    "the Z of ReferenceLine sets the elevation of a new beam.");
            }
        }

        protected override int GeometryInputCount => 1;

        protected override void AddGeometryInputs()
        {
            inManager.AddLineParameter(
                "ReferenceLine",
                "ReferenceLine",
                "The reference line of each new beam (Create): its start and end point, and the Z of its start " +
                "point is the elevation of the reference line."
                    .WithTypeName("lineList"),
                GH_ParamAccess.list);
        }

        protected override void AddGeometryOutputs()
        {
            outManager.AddLineParameter(
                "ReferenceLine",
                "ReferenceLine",
                "The reference line of each beam, at the elevation of its reference line."
                    .WithTypeName("lineList"),
                GH_ParamAccess.list);
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = new List<JObject>();
            var lines = new List<Line>();
            da.GetDataList(firstInputIndex, lines);
            for (var i = 0; i < lines.Count; i++)
            {
                var line = lines[i];
                if (!line.IsValid || (line.From.X == line.To.X && line.From.Y == line.To.Y))
                {
                    this.AddError($"ReferenceLine item {i} has no length in X and Y.");
                    return false;
                }

                geometry.Add(new JObject
                {
                    ["begCoordinate"] = new JObject { ["x"] = line.From.X, ["y"] = line.From.Y },
                    ["endCoordinate"] = new JObject { ["x"] = line.To.X, ["y"] = line.To.Y },
                    ["zCoordinate"] = line.From.Z
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

        private static Line? ReferenceLineOf(
            JObject details)
        {
            if (!(details?["begCoordinate"] is JObject beg) ||
                !(details["endCoordinate"] is JObject end))
            {
                return null;
            }

            var z = (double?) details["zCoordinate"] ?? 0.0;
            return new Line(
                new Point3d((double) beg["x"], (double) beg["y"], z),
                new Point3d((double) end["x"], (double) end["y"], z));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateBeams;

        public override Guid ComponentGuid =>
            new Guid("6b3f9d2e-4a71-4c58-9e0b-b7e2a41c5d93");
    }
}
