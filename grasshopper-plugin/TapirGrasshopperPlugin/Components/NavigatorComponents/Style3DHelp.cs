using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    // The 3D style fields of Get3DStyleDetails, Create3DStyles and
    // Change3DStyles besides the name, which the components take and give as
    // one JSON object per style.
    public static class Style3DHelp
    {
        public static readonly string[] SettingKeys =
        {
            "engineId",
            "model3D",
            "transparency",
            "monochromeEnabled",
            "monochromeSurfaceColor",
            "monochromeContourColor",
            "backgroundAsInRendering",
            "chainBackgroundColors",
            "backgroundSkyColor",
            "backgroundColor",
            "shadingContours",
            "vectorialSunShadow",
            "vectorialHatching",
            "contourThickness",
            "silhouetteThickness",
            "vectorialHatchingThickness",
            "shadowContourThickness",
            "castShadowPercent",
            "shadingPercent",
            "shadowElevation",
            "show3DHotSpotsOnly"
        };

        public static string SettingsDoc =>
            "One JSON object per style with the 3D style settings: " +
            "engineId (integer), model3D (integer, API_3DModelTypeID), transparency, monochromeEnabled, " +
            "monochromeSurfaceColor and monochromeContourColor ({\"red\", \"green\", \"blue\"} 0..1), " +
            "backgroundAsInRendering, chainBackgroundColors, backgroundSkyColor, backgroundColor (colors), " +
            "shadingContours (integer, API_ShadingContoursID), vectorialSunShadow (integer, API_VectorShadowID), " +
            "vectorialHatching, contourThickness, silhouetteThickness, vectorialHatchingThickness, " +
            "shadowContourThickness (pixels), castShadowPercent, shadingPercent (percent), shadowElevation " +
            "(number) and show3DHotSpotsOnly.";

        public static void AddInputs(
            Component component,
            string namesDescription,
            string settingsDescription)
        {
            component.InTexts(
                "Names",
                namesDescription);

            component.InTexts(
                "Settings_JSON",
                settingsDescription + " " + SettingsDoc +
                " Input only 1 to use the same settings for all styles. Optional.");

            component.SetOptionality(new[] { 1 });
        }

        // The styles array of Create3DStyles and Change3DStyles from the Names
        // and Settings_JSON inputs; null after an error.
        public static JArray ReadStyles(
            GH_ActiveObject component,
            IGH_DataAccess da)
        {
            var names = new List<string>();
            if (!da.GetDataList(
                    0,
                    names))
            {
                return null;
            }

            var settings = new List<string>();
            da.GetDataList(
                1,
                settings);

            if (!NavigatorComponentHelp.CheckCount(
                    component,
                    "Settings_JSON",
                    settings.Count,
                    "Names",
                    names.Count))
            {
                return null;
            }

            var styles = new JArray();

            for (var i = 0; i < names.Count; i++)
            {
                if (!NavigatorComponentHelp.TryParseObject(
                        component,
                        "Settings_JSON",
                        NavigatorComponentHelp.GetAt(
                            settings,
                            i),
                        SettingKeys,
                        out JObject style))
                {
                    return null;
                }

                style.AddFirst(
                    new JProperty(
                        "name",
                        names[i]));
                styles.Add(style);
            }

            return styles;
        }

        // The settings of a style of Get3DStyleDetails as the JSON object the
        // Settings_JSON input of Create3DStyles and Change3DStyles accepts.
        public static string SettingsJson(
            JToken style)
        {
            if (!(style is JObject styleObject))
            {
                return null;
            }

            var settings = new JObject();
            foreach (var key in SettingKeys)
            {
                if (styleObject[key] != null)
                {
                    settings[key] = styleObject[key];
                }
            }

            return NavigatorComponentHelp.JsonText(settings);
        }
    }
}
