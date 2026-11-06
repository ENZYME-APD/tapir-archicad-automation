using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Object tool: reads, writes and creates objects with the settings of
    // the Object Settings dialog, from ToolSettings/Object.json. Origin and
    // LibraryPart define a new object (its insertion point, Z included, and
    // the library part placed there). CreateObjects and ModifyObjects share
    // one apply function on the Add-On side (ApplyObjectLampDetails), so they
    // take the same settings. ModifyObjects cannot change the library part;
    // Execute writes the Home Story with SetDetailsOfElements.
    public class ObjectsComponent : ElementToolComponent
    {
        protected override string ToolName => "Object";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateObjects";
        protected override string CreateArrayKey => "objectsData";
        protected override string ModifyCommand => "ModifyObjects";
        protected override string ModifyArrayKey => "objectsWithDetails";

        // The keys of BuildObjectLampDetailFields (ElementCreationCommands.cpp,
        // without the Lamp-only ones), which the CreateObjects and the
        // ModifyObjects item schemas both take, with coordinates and dimensions.
        private static readonly string[] DetailKeys =
        {
            "angle", "coordinates", "dimensions", "fixPoint", "isAutoOnStoryVisibility", "lineTypeId",
            "linkToSettings", "offset", "pen", "reflected", "sectionContourPen", "sectionFillBackgroundPen",
            "sectionFillId", "sectionFillPen", "surfaceId", "useFixedAngle", "useFixSize", "useObjectLineTypes",
            "useObjectMaterials", "useObjectPens", "useObjectSectionAttributes", "visibility"
        };

        private static readonly ISet<string> CreateObjectsKeys = new HashSet<string>(
            DetailKeys.Concat(new[] { "favoriteName", "floorIndex", "id", "libraryPartName" }));

        private static readonly ISet<string> ModifyObjectsKeys = new HashSet<string>(DetailKeys);

        protected override ISet<string> CreateKeys => CreateObjectsKeys;
        protected override ISet<string> ModifyKeys => ModifyObjectsKeys;

        // The Object Settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Preview and Positioning",
            "Floor Plan and Section",
            "Model"
        };

        public ObjectsComponent()
            : base(
                "Objects",
                "Reads (Refresh), writes (Execute) and creates (Create) objects, with the settings of the " +
                "Object Settings dialog as one JSON object per dialog panel. A new object places LibraryPart at " +
                "Origin; it starts from the Object tool defaults, or from FavoriteName, and the given settings " +
                "override them.")
        {
        }

        protected override int GeometryInputCount => 2;

        protected override void AddGeometryInputs()
        {
            InPoints(
                "Origin",
                "Insertion point of each new object (Create); Z is its elevation, and its Home Story is the " +
                "story at that elevation unless Home Story is given.");
            InTexts(
                "LibraryPart",
                "Name of the library part each new object places (Create): one for all objects or one per " +
                "object.");
        }

        protected override void AddGeometryOutputs()
        {
            OutPoints(
                "Origin",
                "Insertion point of each object; Z is its elevation.");
            OutTexts(
                "LibraryPart",
                "Name of the library part of each object.");
        }

        // One object per Origin point; LibraryPart holds one name for all of
        // them or one per point.
        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = null;
            var origins = new List<Point3d>();
            var names = new List<string>();
            da.GetDataList(firstInputIndex, origins);
            da.GetDataList(firstInputIndex + 1, names);

            if (origins.Count == 0 && names.Count == 0)
            {
                geometry = new List<JObject>();
                return true;
            }
            if (origins.Count == 0 || names.Count == 0)
            {
                this.AddError("Create needs both Origin and LibraryPart.");
                return false;
            }
            if (names.Count != 1 && names.Count != origins.Count)
            {
                this.AddError(
                    $"Origin has {origins.Count} points and LibraryPart {names.Count} names: give one name for " +
                    "all objects or one per point.");
                return false;
            }

            geometry = new List<JObject>();
            for (var i = 0; i < origins.Count; i++)
            {
                var name = names.Count == 1 ? names[0] : names[i];
                if (string.IsNullOrWhiteSpace(name))
                {
                    this.AddError($"LibraryPart item {(names.Count == 1 ? 0 : i)} is empty.");
                    geometry = null;
                    return false;
                }
                geometry.Add(new JObject
                {
                    ["coordinates"] = new JObject
                    {
                        ["x"] = origins[i].X,
                        ["y"] = origins[i].Y,
                        ["z"] = origins[i].Z
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

        private static Point3d? OriginOf(
            JObject details)
        {
            if (!(details?["origin"] is JObject origin))
            {
                return null;
            }
            return new Point3d(
                (double) origin["x"],
                (double) origin["y"],
                (double?) origin["z"] ?? 0.0);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateObjects;

        public override Guid ComponentGuid =>
            new Guid("7b1e4c92-3a5d-4f08-b6e1-9d2c5a8f0e43");
    }
}
