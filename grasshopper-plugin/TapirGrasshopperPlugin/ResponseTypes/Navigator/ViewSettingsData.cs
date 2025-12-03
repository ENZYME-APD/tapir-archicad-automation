using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Navigator
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