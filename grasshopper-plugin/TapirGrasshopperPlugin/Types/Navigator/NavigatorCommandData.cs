using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Navigator
{
    public class PublishPublisherSetParameters
    {
        [JsonProperty("publisherSetName")]
        public string PublisherSetName;

        [JsonProperty(
            "outputPath",
            NullValueHandling = NullValueHandling.Ignore)]
        public string OutputPath;

        [JsonProperty(
            "selectedNavigatorItemIds",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<NavigatorGuidWrapper> SelectedNavigatorItemIds;
    }

    public class NavigatorItemWithRotation
    {
        [JsonProperty("navigatorItemId")]
        public NavigatorGuid NavigatorItemId;

        [JsonProperty("rotation")]
        public double Rotation;
    }

    public class SetViewRotationParameters
    {
        [JsonProperty("navigatorItemIdsWithRotation")]
        public List<NavigatorItemWithRotation> NavigatorItemIdsWithRotation;
    }

    public class ViewToClone
    {
        [JsonProperty("navigatorItemId")]
        public NavigatorGuid NavigatorItemId;

        [JsonProperty(
            "parentNavigatorItemId",
            NullValueHandling = NullValueHandling.Ignore)]
        public NavigatorGuid ParentNavigatorItemId;
    }

    public class CloneProjectMapItemToViewMapParameters
    {
        [JsonProperty("viewsData")]
        public List<ViewToClone> ViewsData;
    }

    public class CutPlane
    {
        [JsonProperty("pa")]
        public double Pa;

        [JsonProperty("pb")]
        public double Pb;

        [JsonProperty("pc")]
        public double Pc;

        [JsonProperty("pd")]
        public double Pd;
    }

    public class Set3DCutPlanesParameters
    {
        [JsonProperty("cutPlanes")]
        public List<CutPlane> CutPlanes;
    }
}
