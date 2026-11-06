using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Lamp tool: reads, writes and creates lamps with the settings of the
    // Lamp Settings dialog, from ToolSettings/Lamp.json. Origin places a new
    // lamp (its Z gives the home story and the elevation above it, as in
    // CreateLamps) and LibraryPartName names the lamp library part to place,
    // which CreateLamps requires and the dialog's panels do not hold.
    // CreateLamps and ModifyLamps share one apply function on the Add-On side
    // (ApplyObjectLampDetails), so they take the same settings; only the
    // library part, the Home Story, the favorite and the Element ID are
    // Create-only (the Home Story of an existing lamp is written with
    // SetDetailsOfElements).
    public class LampsComponent : ElementToolComponent
    {
        protected override string ToolName => "Lamp";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateLamps";
        protected override string CreateArrayKey => "lampsData";
        protected override string ModifyCommand => "ModifyLamps";
        protected override string ModifyArrayKey => "lampsWithDetails";

        // The keys of BuildObjectLampDetailFields (ElementCreationCommands.cpp,
        // with isLamp), which the CreateLamps and the ModifyLamps item schemas
        // take, together with coordinates and dimensions.
        private static readonly string[] DetailKeys =
        {
            "angle", "coordinates", "dimensions", "fixPoint", "isAutoOnStoryVisibility", "lightColor",
            "lightIsOn", "lineTypeId", "linkToSettings", "offset", "pen", "reflected", "sectionContourPen",
            "sectionFillBackgroundPen", "sectionFillId", "sectionFillPen", "surfaceId", "useFixedAngle",
            "useFixSize", "useObjectLineTypes", "useObjectMaterials", "useObjectPens",
            "useObjectSectionAttributes", "visibility"
        };

        private static readonly ISet<string> CreateLampsKeys = new HashSet<string>(
            DetailKeys.Concat(new[] { "favoriteName", "floorIndex", "id", "libraryPartName" }));

        private static readonly ISet<string> ModifyLampsKeys = new HashSet<string>(DetailKeys);

        protected override ISet<string> CreateKeys => CreateLampsKeys;
        protected override ISet<string> ModifyKeys => ModifyLampsKeys;

        // The panels of the Lamp Settings dialog that hold settings of the
        // table, in the table's order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Preview and Positioning",
            "Floor Plan and Section",
            "Model",
            "Light Settings"
        };

        public LampsComponent()
            : base(
                "Lamps",
                "Reads (Refresh), writes (Execute) and creates (Create) lamps, with the settings of the " +
                "Lamp Settings dialog as one JSON object per dialog panel. A new lamp of the library part " +
                "LibraryPartName is placed at Origin (its Z sets the home story and the elevation); it starts " +
                "from the Lamp tool defaults, or from FavoriteName, and the given settings override them.")
        {
        }

        protected override int GeometryInputCount => 2;

        protected override void AddGeometryInputs()
        {
            InPoints(
                "Origin",
                "The origin of each new lamp (Create). Its Z is the elevation: the home story is the story " +
                "it is on, unless Home Story is given in PreviewAndPositioning_JSON.");
            InTexts(
                "LibraryPartName",
                "The name of the lamp library part of the new lamps (Create): one for all lamps or one per lamp.");
        }

        protected override void AddGeometryOutputs()
        {
            OutPoints(
                "Origin",
                "The origin of each lamp.");
            OutTexts(
                "LibraryPartName",
                "The name of the library part of each lamp.");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = null;
            var points = new List<Point3d>();
            var names = new List<string>();
            da.GetDataList(firstInputIndex, points);
            da.GetDataList(firstInputIndex + 1, names);

            if (points.Count == 0)
            {
                geometry = new List<JObject>();
                return true;
            }
            if (names.Count == 0)
            {
                this.AddError("Create needs LibraryPartName, the lamp library part to place.");
                return false;
            }
            if (names.Count != 1 && names.Count != points.Count)
            {
                this.AddError(
                    $"LibraryPartName has {names.Count} items for {points.Count} origins: give one for all " +
                    "lamps or one per lamp.");
                return false;
            }

            geometry = new List<JObject>();
            for (var i = 0; i < points.Count; i++)
            {
                var name = names.Count == 1 ? names[0] : names[i];
                if (string.IsNullOrEmpty(name))
                {
                    this.AddError($"LibraryPartName item {(names.Count == 1 ? 0 : i)} is empty.");
                    geometry = null;
                    return false;
                }
                geometry.Add(new JObject
                {
                    ["coordinates"] = new JObject
                    {
                        ["x"] = points[i].X,
                        ["y"] = points[i].Y,
                        ["z"] = points[i].Z
                    },
                    ["libraryPartName"] = name
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
                details.Select(d => d?["libPart"]?["name"]?.ToString()));
        }

        private static GH_Point OriginOf(
            JObject details)
        {
            if (!(details?["origin"] is JObject origin))
            {
                return null;
            }

            return new GH_Point(
                new Point3d(
                    (double?) origin["x"] ?? 0.0,
                    (double?) origin["y"] ?? 0.0,
                    (double?) origin["z"] ?? 0.0));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLamps;

        public override Guid ComponentGuid =>
            new Guid("8b4e2f6a-3c1d-4e7f-a925-6d0b1c7e4f58");
    }
}
