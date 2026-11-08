using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // The settings of the Opening tool, as the opening commands (GetOpeningDetails,
    // SetOpeningDetails, Get/SetOpeningFloorPlanParameters, Get/SetOpeningDefault,
    // PlaceOpenings) read and write them: the extrusion parameters and the floor
    // plan parameters, each one JSON object with the Add-On's names. There is no
    // ToolSettings table of the Opening tool yet, so the API names are used.
    internal static class OpeningSettingsHelp
    {
        public const string ExtrusionParametersDescription =
            "The size and extrusion settings, as a JSON object with the names GetOpeningDetails returns: " +
            "constraint (ForcedVertical, ForcedHorizontal, Aligned, Free), limitType (Infinite, Finite, " +
            "HalfInfinite), linkedStatus (Linked, NotLinked), shapeType (Rectangular, Circular, Polygonal), " +
            "width, height, anchor (TopLeft ... BottomRight), anchorAltitude, extrusionStartOffset, " +
            "finiteBodyLength, isExtrusionSurfaceOverridden, extrusionSurfaceId, isEndSurfaceOverridden, " +
            "endSurfaceId, areOverriddenSurfacesLinked. Only the given keys are changed.";

        public const string FloorPlanParametersDescription =
            "The floor plan settings, as a JSON object with the names GetOpeningFloorPlanParameters returns: " +
            "floorPlanDisplayMode, dividedOpeningMode, openEdgeStyle, showReferenceAxis, referenceAxisPen, " +
            "referenceAxisLineTypeId, referenceAxisOverhang, cutSurfacesUseLineOfCutElements, cutSurfacesLinePen, " +
            "cutSurfacesLineTypeId, outlinesUseLineOfCutElements, outlinesUncutLinePen, outlinesUncutLineTypeId, " +
            "outlinesOverheadLinePen, outlinesOverheadLineTypeId, useCoverFills, coverFillsUseFillsOfCutElements, " +
            "coverFillId, coverFillPen, coverFillBackgroundPen, coverFillOrientation, coverFillTransformation, " +
            "cutSymbolLibPartId, unCutSymbolLibPartId, and clearCutSymbol / clearUnCutSymbol (true removes the " +
            "symbol). Attributes are {\"guid\": ...} objects. Only the given keys are changed.";

        // The keys of the extrusion parameters (OpeningExtrusionParametersInputSchema).
        public static readonly ISet<string> ExtrusionKeys = new HashSet<string>
        {
            "constraint", "limitType", "linkedStatus", "shapeType", "width", "height", "anchor",
            "anchorAltitude", "extrusionStartOffset", "finiteBodyLength", "isExtrusionSurfaceOverridden",
            "extrusionSurfaceId", "isEndSurfaceOverridden", "endSurfaceId", "areOverriddenSurfacesLinked"
        };

        // The keys of the floor plan parameters (OpeningFloorPlanParametersSchemaProperties
        // and the two clear flags of the setters).
        public static readonly ISet<string> FloorPlanKeys = new HashSet<string>
        {
            "floorPlanDisplayMode", "dividedOpeningMode", "openEdgeStyle", "showReferenceAxis",
            "referenceAxisPen", "referenceAxisLineTypeId", "referenceAxisOverhang",
            "cutSurfacesUseLineOfCutElements", "cutSurfacesLinePen", "cutSurfacesLineTypeId",
            "outlinesUseLineOfCutElements", "outlinesUncutLinePen", "outlinesUncutLineTypeId",
            "outlinesOverheadLinePen", "outlinesOverheadLineTypeId", "useCoverFills",
            "coverFillsUseFillsOfCutElements", "coverFillId", "coverFillPen", "coverFillBackgroundPen",
            "coverFillOrientation", "coverFillTransformation", "cutSymbolLibPartId", "unCutSymbolLibPartId",
            "clearCutSymbol", "clearUnCutSymbol"
        };

        // Parses a settings JSON input. An empty text gives null (nothing to
        // change); invalid JSON or an unknown key adds an error and gives false.
        public static bool TryParse(
            GH_ActiveObject owner,
            string text,
            string inputName,
            ISet<string> keys,
            out JObject settings)
        {
            settings = null;
            if (string.IsNullOrWhiteSpace(text))
            {
                return true;
            }

            try
            {
                settings = JObject.Parse(text);
            }
            catch (JsonException ex)
            {
                owner.AddError($"{inputName} is not a valid JSON object: {ex.Message}");
                return false;
            }

            foreach (var property in settings.Properties())
            {
                if (!keys.Contains(property.Name))
                {
                    owner.AddError($"Unknown key \"{property.Name}\" in {inputName}.");
                    return false;
                }
            }

            return true;
        }

        // A JSON object of the response as an output text; null when absent.
        public static string ToText(
            JToken settings)
        {
            return settings is JObject obj
                ? obj.ToString(Formatting.None)
                : null;
        }
    }
}
