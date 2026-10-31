using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Navigator
{
    public class ViewSettingsData
    {
        [JsonProperty("viewSettings")]
        public List<ViewSettingsOrError> ViewSettings { get; set; }

        public static ViewSettingsData FromResponse(
            JObject baseResponse)
        {
            if (!baseResponse.ContainsKey("viewSettings"))
            {
                throw new Exception(
                    $"Invalid response object in {nameof(ViewSettingsData)}: No 'viewSettings' key.");
            }
            else
            {
                var viewSettings =
                    baseResponse.GetValue("viewSettings") as JArray;

                if (viewSettings == null)
                {
                    throw new Exception(
                        $"Invalid response object in {nameof(ViewSettingsData)}: 'viewSettings' is not a list.");
                }

                return new ViewSettingsData
                {
                    ViewSettings = viewSettings
                        .Values<JObject>()
                        .Select(ViewSettingsOrError.Deserialize)
                        .ToList()
                };
            }
        }
    }

    public class ViewSettingsOrError
    {
        public static ViewSettingsOrError Deserialize(
            JObject jObject)
        {
            if (jObject.ContainsKey("error"))
            {
                return jObject.ToObject<ErrorItem>();
            }
            else
            {
                return jObject.ToObject<ViewSettings>();
            }
        }
    }

    public class ViewSettings : ViewSettingsOrError
    {
        [JsonProperty("modelViewOptions")]
        public string ModelViewOptions { get; set; }

        [JsonProperty("layerCombination")]
        public string LayerCombination { get; set; }

        [JsonProperty("dimensionStyle")]
        public string DimensionStyle { get; set; }

        [JsonProperty("penSetName")]
        public string PenSetName { get; set; }

        [JsonProperty("graphicOverrideCombination")]
        public string GraphicOverrideCombination { get; set; }

        // The properties below are optional on the wire: older add-on
        // versions do not send them, and SetViewSettings must not receive
        // nulls for the ones a component leaves unset - hence the nullable
        // types and NullValueHandling.Ignore.

        [JsonProperty("drawingScale", NullValueHandling = NullValueHandling.Ignore)]
        public int? DrawingScale { get; set; }

        [JsonProperty("saveZoom", NullValueHandling = NullValueHandling.Ignore)]
        public bool? SaveZoom { get; set; }

        [JsonProperty("ignoreSavedZoom", NullValueHandling = NullValueHandling.Ignore)]
        public bool? IgnoreSavedZoom { get; set; }

        [JsonProperty("zoom", NullValueHandling = NullValueHandling.Ignore)]
        public ViewZoomBox Zoom { get; set; }

        [JsonProperty("saveFlags", NullValueHandling = NullValueHandling.Ignore)]
        public ViewSaveFlags SaveFlags { get; set; }

        [JsonProperty("rotation", NullValueHandling = NullValueHandling.Ignore)]
        public double? Rotation { get; set; }

        // Read-only on the wire: GetViewSettings sends it, SetViewSettings ignores it.
        [JsonProperty("transformationMatrix", NullValueHandling = NullValueHandling.Ignore)]
        public List<double> TransformationMatrix { get; set; }

        [JsonProperty("structureDisplay", NullValueHandling = NullValueHandling.Ignore)]
        public string StructureDisplay { get; set; }

        [JsonProperty("renovationFilterGuid", NullValueHandling = NullValueHandling.Ignore)]
        public RenovationFilterGuid RenovationFilterGuid { get; set; }

        [JsonProperty("d3styleName", NullValueHandling = NullValueHandling.Ignore)]
        public string D3StyleName { get; set; }

        [JsonProperty("renderingSceneName", NullValueHandling = NullValueHandling.Ignore)]
        public string RenderingSceneName { get; set; }

        [JsonProperty("usePhotoRendering", NullValueHandling = NullValueHandling.Ignore)]
        public bool? UsePhotoRendering { get; set; }
    }

    // The "stored in this view" checkboxes of the View Settings dialog. Every flag
    // is optional for SetViewSettings: a flag left null keeps its current value.
    public class ViewSaveFlags
    {
        [JsonProperty("zoom", NullValueHandling = NullValueHandling.Ignore)]
        public bool? Zoom { get; set; }

        [JsonProperty("displayOptions", NullValueHandling = NullValueHandling.Ignore)]
        public bool? DisplayOptions { get; set; }

        [JsonProperty("layers", NullValueHandling = NullValueHandling.Ignore)]
        public bool? Layers { get; set; }

        [JsonProperty("drawingScale", NullValueHandling = NullValueHandling.Ignore)]
        public bool? DrawingScale { get; set; }

        [JsonProperty("dimensions", NullValueHandling = NullValueHandling.Ignore)]
        public bool? Dimensions { get; set; }

        [JsonProperty("penSet", NullValueHandling = NullValueHandling.Ignore)]
        public bool? PenSet { get; set; }

        [JsonProperty("structureDisplay", NullValueHandling = NullValueHandling.Ignore)]
        public bool? StructureDisplay { get; set; }
    }

    public class ViewZoomBox
    {
        [JsonProperty("xMin")]
        public double XMin { get; set; }

        [JsonProperty("yMin")]
        public double YMin { get; set; }

        [JsonProperty("xMax")]
        public double XMax { get; set; }

        [JsonProperty("yMax")]
        public double YMax { get; set; }
    }

    public class RenovationFilterGuid : GuidObject<RenovationFilterGuid>
    {
    }

    public class ErrorItem : ViewSettingsOrError
    {
        [JsonProperty("error")]
        public Error Error { get; set; }
    }

    public class SetViewSettingsObject
    {
        [JsonProperty("navigatorItemId")]
        public NavigatorGuid NavigatorGuid { get; set; }

        [JsonProperty("viewSettings")]
        public ViewSettings ViewSettings { get; set; }
    }
}