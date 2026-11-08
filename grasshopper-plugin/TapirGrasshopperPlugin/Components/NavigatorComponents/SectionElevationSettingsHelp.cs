using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    // The settings of Section and Elevation elements, as
    // GetSectionsAndElevationsDetails reads them and
    // SetSectionsAndElevationsDetails writes them: one JSON object with the
    // Add-On's names (SectionElevationSettableDetails). There is no
    // ToolSettings table of the Section tool yet, so the API names are used.
    internal static class SectionElevationSettingsHelp
    {
        public const string DetailsDescription =
            "The settings of the Section or Elevation, as a JSON object with the names " +
            "GetSectionsAndElevationsDetails returns in its Details output, e.g. linePenIndex, textPenIndex, " +
            "lineTypeId, horizontalRange (Infinite, Limited, ZeroDepth), verticalRange (Infinite, Limited, " +
            "FitToZoneRange), verticalRangeMin, verticalRangeMax, markerSize, showMarkerAtBeginning, " +
            "showMarkerAtEnd, textSize, fontIndex, showOnStories (All, OneStory, InRange, PartRange), " +
            "storyToShowOn, markerShowType (MiddleMarker, WingMarkers), intendedPlanConnectivity (Online, " +
            "Offline, Drawing), the Model Effects, distant area and story handle settings. Attributes are " +
            "{\"guid\": ...} objects. Only the given keys are changed. The name and the reference ID are " +
            "not part of it: ModifySections changes them.";

        // The keys of SectionElevationSettableDetails.
        public static readonly ISet<string> SettableKeys = new HashSet<string>
        {
            "linePenIndex", "textPenIndex", "lineTypeId", "sectionLinePenIndex", "sectionFillPenIndex",
            "sectionFillBackgroundPenIndex", "horizontalRange", "verticalRange", "verticalRangeStoryBaseNumber",
            "relativeToStory", "verticalRangeMin", "verticalRangeMax", "markerSize", "showMarkerAtBeginning",
            "showMarkerAtEnd", "useElementPens", "showLineAtBeginning", "showLineAtMiddle", "showLineAtEnd",
            "textSize", "fontIndex", "effectVectorHatch", "effectVectorShadow", "effectSunFrom3D",
            "shadowFillId", "shadowFillPenIndex", "shadowFillBackgroundPenIndex", "limitLineTypeId",
            "limitLinePenIndex", "continuous", "sunAzimuth", "sunAltitude", "useOneModelPen", "modelPenIndex",
            "modelUncutSurfaceFillType", "modelUncutSurfaceFillBackgroundPenIndex", "useUncutSurfaceFill",
            "transparent", "markedDistantArea", "useOneDistantAreaPen", "distantAreaPenIndex",
            "distantAreaUncutSurfaceFillBackgroundPenIndex", "useUncutSurfaceFillInDistantArea",
            "storyHandleAppearance", "storyHandleLineTypeId", "storyHandleLinePenIndex",
            "storyHandleMarkerPenIndex", "storyHandleUseSymbolPens", "storyHandleLeftMarkerVisible",
            "storyHandleRightMarkerVisible", "storyHandleLineVisible", "storyHandleLeftMarkerOffset",
            "storyHandleRightMarkerOffset", "storyHandleMarkerFontIndex", "storyHandleMarkerTextSize",
            "storyHandleMarkerSize", "boundaryDisplayMode", "boundaryPenIndex", "boundaryLineTypeId",
            "showOnStories", "storyToShowOn", "markerShowType", "intendedPlanConnectivity"
        };

        // The keys of SectionElevationDetails that are output on their own,
        // not in the details JSON.
        public static readonly ISet<string> OwnOutputKeys = new HashSet<string>
        {
            "name", "id"
        };

        // Splits the details read by GetSectionsAndElevationsDetails into the
        // settable ones (SetSectionsAndElevationsDetails takes them back) and
        // the read-only ones; the name and the ID go to neither.
        public static void Split(
            JObject details,
            out JObject settable,
            out JObject readOnly)
        {
            settable = new JObject();
            readOnly = new JObject();
            foreach (var property in details.Properties())
            {
                if (OwnOutputKeys.Contains(property.Name))
                {
                    continue;
                }

                if (SettableKeys.Contains(property.Name))
                {
                    settable[property.Name] = property.Value;
                }
                else
                {
                    readOnly[property.Name] = property.Value;
                }
            }
        }

        public static string ToText(
            JObject settings)
        {
            return settings.ToString(Formatting.None);
        }
    }
}
