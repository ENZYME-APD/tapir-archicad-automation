using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Project
{
    public class ProjectInfo
    {
        [JsonProperty("isUntitled")]
        public bool IsUntitled { get; set; }

        [JsonProperty("isTeamwork")]
        public bool IsTeamwork { get; set; }

        [JsonProperty("projectLocation")]
        public string ProjectLocation { get; set; }

        [JsonProperty("projectPath")]
        public string ProjectPath { get; set; }

        [JsonProperty("projectName")]
        public string ProjectName { get; set; }
    }

    public class ProjectInfoField
    {
        [JsonProperty("projectInfoId")]
        public string ProjectInfoId { get; set; }

        [JsonProperty("projectInfoName")]
        public string ProjectInfoName { get; set; }

        [JsonProperty("projectInfoValue")]
        public string ProjectInfoValue { get; set; }

        public ProjectInfoField()
        {
        }

        public ProjectInfoField(
            string id,
            string value)
        {
            ProjectInfoId = id;
            ProjectInfoValue = value;
        }
    }

    public class ProjectInfoFields
    {
        [JsonProperty("fields")]
        public List<ProjectInfoField> Fields { get; set; }
    }

    public class Location
    {
        [JsonProperty("longitude", NullValueHandling = NullValueHandling.Ignore)]
        public double? Longitude;

        [JsonProperty("latitude", NullValueHandling = NullValueHandling.Ignore)]
        public double? Latitude;

        [JsonProperty("altitude", NullValueHandling = NullValueHandling.Ignore)]
        public double? Altitude;

        [JsonProperty("north", NullValueHandling = NullValueHandling.Ignore)]
        public double? North;
    }

    public class SurveyPointPosition
    {
        [JsonProperty("eastings", NullValueHandling = NullValueHandling.Ignore)]
        public double? Eastings;

        [JsonProperty("northings", NullValueHandling = NullValueHandling.Ignore)]
        public double? Northings;

        [JsonProperty("elevation", NullValueHandling = NullValueHandling.Ignore)]
        public double? Elevation;
    }

    public class GeoReferencingParameters
    {
        [JsonProperty("crsName", NullValueHandling = NullValueHandling.Ignore)]
        public string CrsName;

        [JsonProperty("description", NullValueHandling = NullValueHandling.Ignore)]
        public string Description;

        [JsonProperty("geodeticDatum", NullValueHandling = NullValueHandling.Ignore)]
        public string GeodeticDatum;

        [JsonProperty("verticalDatum", NullValueHandling = NullValueHandling.Ignore)]
        public string VerticalDatum;

        [JsonProperty("mapProjection", NullValueHandling = NullValueHandling.Ignore)]
        public string MapProjection;

        [JsonProperty("mapZone", NullValueHandling = NullValueHandling.Ignore)]
        public string MapZone;
    }

    public class SurveyPoint
    {
        [JsonProperty("position")]
        public SurveyPointPosition Position;

        [JsonProperty("geoReferencingParameters")]
        public GeoReferencingParameters GeoReferencingParams;
    }

    public class ProjectLocation
    {
        [JsonProperty("projectLocation")]
        public Location Loc;

        [JsonProperty("surveyPoint")]
        public SurveyPoint Survey;
    }
}