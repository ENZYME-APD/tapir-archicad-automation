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
    // The Hotspot tool: reads and creates hotspots with the settings of the
    // Hotspot Settings dialog, from ToolSettings/Hotspot.json. Position and
    // Height define a new hotspot. There is no ModifyHotspots command:
    // Execute writes the Color (and a position or height given in the JSON)
    // through SetDetailsOfElements' typeSpecificDetails, and the Element ID,
    // the layer, the draw order and the story ({"floorIndex": ...}).
    public class HotspotsComponent : ElementToolComponent
    {
        protected override string ToolName => "Hotspot";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateHotspots";
        protected override string CreateArrayKey => "hotspotsData";
        protected override string ModifyCommand => null;
        protected override string ModifyArrayKey => null;

        // The item properties of the CreateHotspots input schema
        // (ElementCreationCommands.cpp), with floorIndex for its floorInd
        // (see PrepareSettings).
        private static readonly ISet<string> CreateHotspotsKeys = new HashSet<string>
        {
            "favoriteName", "floorIndex", "height", "id", "layerIndex", "penIndex", "position"
        };

        protected override ISet<string> CreateKeys => CreateHotspotsKeys;
        protected override ISet<string> ModifyKeys { get; } = new HashSet<string>();

        // SetDetailsOfElements writes these of a hotspot from
        // typeSpecificDetails (HotspotSettings, ElementCommands.cpp).
        protected override ISet<string> TypeSpecificDetailsKeys { get; } = new HashSet<string>
        {
            "penIndex", "position", "height"
        };

        // typeSpecificDetails is one of several settings schemas (oneOf):
        // without penIndex, position and height also match TextSettings
        // (height alone also WallSettings), so Archicad would refuse them.
        protected override void PrepareTypeSpecificDetails(
            JObject typeSpecificDetails,
            ISet<string> notes)
        {
            if (typeSpecificDetails["penIndex"] != null)
            {
                return;
            }
            var dropped = new List<string>();
            foreach (var key in new[] { "position", "height" })
            {
                if (typeSpecificDetails.Remove(key))
                {
                    dropped.Add(KeyOf(key));
                }
            }
            if (dropped.Count > 0)
            {
                notes.Add(
                    $"{string.Join(", ", dropped)} of existing hotspots can only be changed together with " +
                    $"{KeyOf("penIndex")}; not changed.");
            }
        }

        // The Hotspot Settings dialog's panels, in its order;
        // append-only (see ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Color"
        };

        public HotspotsComponent()
            : base(
                "Hotspots",
                "Reads (Refresh), modifies (Execute) and creates (Create) hotspots, with the settings of the " +
                "Hotspot Settings dialog as one JSON object per dialog panel. A new hotspot starts from the Hotspot " +
                "tool defaults, or from FavoriteName, and the given settings override them. The story can be given " +
                "as {\"floorIndex\": ...} in any JSON input. Execute writes the given settings and ElementID, " +
                "LayerIndex and DrawOrder to the hotspots in ElementGuid; the Position and Height inputs are used " +
                "by Create only.",
                GroupNames.DocumentTools)
        {
        }

        // The table writes the story as floorInd, the key of CreateHotspots;
        // SetDetailsOfElements (Execute) takes it as floorIndex. It is kept
        // as floorIndex here and renamed back for Create (PrepareCreateItem).
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
                "Position",
                "The location of each new hotspot (Create; only X and Y are used).");
            InNumbers(
                "Height",
                "The Z coordinate of each new hotspot (Create): none, one for all or one per position.");
        }

        protected override void AddGeometryOutputs()
        {
            OutPoints(
                "Position",
                "The location of each hotspot (Z is 0).");
            OutNumberList(
                "Height",
                "The Z coordinate of each hotspot.");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = null;
            var positions = new List<Point3d>();
            da.GetDataList(firstInputIndex, positions);
            var heights = da.OptionalList<double>(firstInputIndex + 1);

            if (!this.CheckSizes(
                    positions.Count,
                    "Position",
                    ("Height", heights.Count)))
            {
                return false;
            }

            geometry = new List<JObject>();
            for (var i = 0; i < positions.Count; i++)
            {
                var item = new JObject
                {
                    ["position"] = new JObject { ["x"] = positions[i].X, ["y"] = positions[i].Y }
                };
                if (heights.Count > 0)
                {
                    item["height"] = heights.At(i);
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
                details.Select(d => JsonOutputHelp.Point(d?["position"])));
            da.SetDataList(
                firstOutputIndex + 1,
                details.Select(d => (double?) d?["height"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateHotspots;

        public override Guid ComponentGuid =>
            new Guid("5b0f3c7e-8a41-4d6e-9c2b-7e13f4a6d905");
    }
}
