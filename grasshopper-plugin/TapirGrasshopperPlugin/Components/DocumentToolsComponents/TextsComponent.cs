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
    // The Text tool: reads, writes and creates texts with the settings of the
    // Text Settings dialog, from ToolSettings/Text.json. Position and Text
    // define a new text; the content is no setting of the dialog, so it is a
    // plain input and output like the position. Execute writes with
    // ModifyTexts, which takes the style only as a whole ("style"), so the
    // flat style keys of the table are moved into it (PrepareSettings).
    public class TextsComponent : ElementToolComponent
    {
        protected override string ToolName => "Text";

        protected override bool UseLabels => true;

        protected override string CreateCommand => "CreateTexts";
        protected override string CreateArrayKey => "textsData";
        protected override string ModifyCommand => "ModifyTexts";
        protected override string ModifyArrayKey => "textsWithDetails";

        // The item properties of the CreateTexts and ModifyTexts input schemas
        // (ElementCreationCommands.cpp).
        private static readonly ISet<string> CreateTextsKeys = new HashSet<string>
        {
            "angle", "coordinate", "favoriteName", "floorIndex", "height", "id", "justification", "layerIndex",
            "pen", "runs", "style", "text"
        };

        private static readonly ISet<string> ModifyTextsKeys = new HashSet<string>
        {
            "coordinate", "floorIndex", "runs", "style", "text"
        };

        protected override ISet<string> CreateKeys => CreateTextsKeys;
        protected override ISet<string> ModifyKeys => ModifyTextsKeys;

        // The Text Settings dialog's panels, in the order of the table;
        // append-only (see ElementToolComponent.JsonGroups).
        protected override IReadOnlyList<string> JsonGroups { get; } = new[]
        {
            "Text Style",
            "Text Block Formatting"
        };

        // The flat keys of CreateTexts and their names in "style"
        // (TextStyleSettableDetails), which is all ModifyTexts takes.
        private static readonly (string Key, string StyleKey)[] FlatStyleKeys =
        {
            ("height", "height"),
            ("pen", "penIndex"),
            ("angle", "angle"),
            ("justification", "justification")
        };

        // The keys GetDetailsOfElements returns in "style" (TextStyleDetails)
        // that no command takes (TextStyleSettableDetails).
        private static readonly string[] ReadOnlyStyleKeys =
        {
            "lineCount", "boxWidth", "boxHeight"
        };

        public TextsComponent()
            : base(
                "Texts",
                "Reads (Refresh), writes (Execute) and creates (Create) texts, with the settings of the Text " +
                "Settings dialog as one JSON object per dialog panel. A new text starts from the Text tool " +
                "defaults, or from FavoriteName, and the given settings override them. The \"style\" key of " +
                "TextStyle_JSON holds every style setting (font, faces, effects, frame, anchor ...); Font Size, " +
                "Text Pencolor, Alignment and Rotation Angle take precedence over the same settings in it. A " +
                "change of the font, pen, faces, size or effects rebuilds the content as one paragraph, which " +
                "makes the text auto-width, and on a text of several runs applies that style to every run. On " +
                "Execute, the content can be changed as {\"text\": ...} and the story as {\"floorIndex\": ...} " +
                "in any JSON input.",
                GroupNames.DocumentTools)
        {
        }

        protected override int GeometryInputCount => 2;

        protected override void AddGeometryInputs()
        {
            InPoints(
                "Position",
                "The anchor point of each new text (Create). Its Z selects the story when no floorIndex is " +
                "given.");
            InTexts(
                "Text",
                "The content of each new text (Create): one for all or one per position. Newlines create " +
                "multiple lines. Needed unless the JSON gives the content as \"runs\", which take precedence.");
        }

        protected override void AddGeometryOutputs()
        {
            OutPoints(
                "Position",
                "The anchor point of each text, at the level of its story.");
            OutTexts(
                "Text",
                "The content of each text.");
        }

        protected override bool TryGetGeometry(
            IGH_DataAccess da,
            int firstInputIndex,
            out List<JObject> geometry)
        {
            geometry = null;
            var positions = new List<Point3d>();
            da.GetDataList(firstInputIndex, positions);
            var texts = da.OptionalList<string>(firstInputIndex + 1);

            if (!this.CheckSizes(
                    positions.Count,
                    "Position",
                    ("Text", texts.Count)))
            {
                return false;
            }

            geometry = new List<JObject>();
            for (var i = 0; i < positions.Count; i++)
            {
                var item = new JObject
                {
                    ["coordinate"] = new JObject
                    {
                        ["x"] = positions[i].X,
                        ["y"] = positions[i].Y,
                        ["z"] = positions[i].Z
                    }
                };
                if (texts.Count > 0 && texts.At(i) != null)
                {
                    item["text"] = texts.At(i);
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
                details.Select(d => PositionOf(d)));
            da.SetDataList(
                firstOutputIndex + 1,
                details.Select(d => d?["text"]?.ToString()));
        }

        private static object PositionOf(
            JObject details)
        {
            var position = details?["position"];
            if (position == null || position.Type == JTokenType.Null)
            {
                return null;
            }

            return new Point3d(
                position.Value<double?>("x") ?? 0.0,
                position.Value<double?>("y") ?? 0.0,
                (double?) details["zCoordinate"] ?? 0.0);
        }

        // ModifyTexts takes the style only inside "style", so the flat keys
        // go there (they take precedence, as in CreateTexts, which takes
        // both); the read-only keys of a Refresh JSON are dropped.
        protected override void PrepareSettings(
            JObject settings,
            ISet<string> notes)
        {
            var styleToken = settings["style"];
            if (styleToken != null && !(styleToken is JObject))
            {
                // Not an object: left to the command, which reports it.
                return;
            }

            var style = styleToken as JObject;
            if (style != null)
            {
                foreach (var key in ReadOnlyStyleKeys)
                {
                    style.Remove(key);
                }
            }

            foreach (var (key, styleKey) in FlatStyleKeys)
            {
                var value = settings[key];
                if (value == null)
                {
                    continue;
                }
                style = style ?? new JObject();
                style[styleKey] = value;
                settings.Remove(key);
            }

            if (style != null)
            {
                settings["style"] = style;
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateTexts;

        public override Guid ComponentGuid =>
            new Guid("5b3e9c7a-2f41-4d8e-9a6c-1e7b04d3f952");
    }
}
