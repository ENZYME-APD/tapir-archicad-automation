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
    // The Polyline tool: reads and creates polylines with the settings of the
    // Polyline settings dialog, from ToolSettings/Polyline.json. Polyline defines
    // a new polyline (a curve; only X and Y are used, line and arc segments
    // are kept, anything else is approximated). There is no ModifyPolylines
    // command, so Execute only writes the Element ID, the layer and the draw
    // order of existing polylines.
    public class PolylinesComponent : ElementToolComponent
    {
        protected override string ToolName => "Polyline";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreatePolylines";
        protected override string CreateArrayKey => "polylinesData";
        protected override string ModifyCommand => null;
        protected override string ModifyArrayKey => null;

        // The item properties of the CreatePolylines input schema
        // (ElementCreationCommands.cpp).
        private static readonly ISet<string> CreatePolylinesKeys = new HashSet<string>
        {
            "arcs", "coordinates", "favoriteName", "floorInd", "id", "layerIndex", "linePenIndex",
            "lineTypeIndex", "penWeightMm", "roomSeparator"
        };

        protected override ISet<string> CreateKeys => CreatePolylinesKeys;
        protected override ISet<string> ModifyKeys { get; } = new HashSet<string>();

        // The Line Settings dialog's panels, in its order; append-only (see
        // ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "General Settings"
        };

        // The index of each line type by its guid, read once per solution
        // when a Line Type is given as the guid a Refresh returns.
        private Dictionary<string, int> _lineTypeIndices;

        public PolylinesComponent()
            : base(
                "Polylines",
                "Reads (Refresh) and creates (Create) polylines, with the settings of the Polyline settings dialog " +
                "as one JSON object per dialog panel. A new polyline starts from the Polyline tool defaults, or " +
                "from FavoriteName, and the given settings override them. The story can be given as " +
                "{\"floorIndex\": ...} in any JSON input. Archicad has no command to modify polylines, so " +
                "Execute only writes ElementID, LayerIndex and DrawOrder.",
                GroupNames.DocumentTools)
        {
        }

        protected override int GeometryInputCount => 1;

        protected override void AddGeometryInputs()
        {
            inManager.AddCurveParameter(
                "Polyline",
                "Polyline",
                "The curve of each new polyline (Create; only X and Y are used). Line and arc segments are " +
                "kept, other segments are approximated with straight ones."
                    .WithTypeName("curveList"),
                GH_ParamAccess.list);
        }

        protected override void AddGeometryOutputs()
        {
            OutCurves(
                "Polyline",
                "The curve of each polyline, at the elevation of its story.");
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
                if (!PolygonHelp.TryToCoordinates(
                        curves[i],
                        false,
                        out var coordinates,
                        out var arcs,
                        out var error))
                {
                    this.AddError($"Polyline item {i}: {error}.");
                    return false;
                }

                var item = new JObject { ["coordinates"] = coordinates };
                if (arcs.Count > 0)
                {
                    item["arcs"] = arcs;
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
                details.Select(
                    d => d == null
                        ? null
                        : PolygonHelp.ToCurve(
                            d["coordinates"],
                            d["arcs"],
                            false,
                            (double?) d["zCoordinate"] ?? 0.0)));
        }

        protected override void BeforeSolveInstance()
        {
            base.BeforeSolveInstance();
            _lineTypeIndices = null;
        }

        // A Refresh returns the Line Type as the line type's guid
        // ({"guid": ...}), CreatePolylines takes its index: the guid is
        // turned into the index, so a JSON read by Refresh can be fed back.
        protected override void PrepareSettings(
            JObject settings,
            ISet<string> notes)
        {
            var lineType = settings["lineTypeIndex"];
            if (lineType == null || lineType.Type == JTokenType.Integer)
            {
                return;
            }
            if (lineType.Type == JTokenType.Float)
            {
                settings["lineTypeIndex"] = (int) Math.Round((double) lineType);
                return;
            }

            var guid = lineType.Type == JTokenType.Object
                ? lineType["guid"]?.ToString()
                : lineType.Type == JTokenType.String
                    ? lineType.ToString()
                    : null;
            if (guid != null && LineTypeIndices().TryGetValue(guid, out var index))
            {
                settings["lineTypeIndex"] = index;
                return;
            }

            settings.Remove("lineTypeIndex");
            notes.Add(
                $"Line Type {lineType.ToString(Newtonsoft.Json.Formatting.None)} is not a line type of the " +
                "project; the default line type was used.");
        }

        private Dictionary<string, int> LineTypeIndices()
        {
            if (_lineTypeIndices != null)
            {
                return _lineTypeIndices;
            }

            _lineTypeIndices = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
            if (TryGetCadResponse(
                    "GetAttributesByType",
                    new JObject { ["attributeType"] = "Line" },
                    ToAddOn,
                    out var response))
            {
                foreach (var attribute in response["attributes"] as JArray ?? new JArray())
                {
                    var guid = attribute["attributeId"]?["guid"]?.ToString();
                    var index = attribute["index"];
                    if (guid != null && index != null && index.Type == JTokenType.Integer)
                    {
                        _lineTypeIndices[guid] = (int) index;
                    }
                }
            }
            return _lineTypeIndices;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreatePolylines;

        public override Guid ComponentGuid =>
            new Guid("5c8e2a47-91d3-4b6f-a0e8-3f7d1c29b654");
    }
}
