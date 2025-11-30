using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Project
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
        [JsonProperty("longitude")]
        public double Longitude;

        [JsonProperty("latitude")]
        public double Latitude;

        [JsonProperty("altitude")]
        public double Altitude;

        [JsonProperty("north")]
        public double North;
    }

    public class SurveyPointPosition
    {
        [JsonProperty("eastings")]
        public double Eastings;

        [JsonProperty("northings")]
        public double Northings;

        [JsonProperty("elevation")]
        public double Elevation;
    }

    public class GeoReferencingParameters
    {
        [JsonProperty("crsName")]
        public string CrsName;

        [JsonProperty("description")]
        public string Description;

        [JsonProperty("geodeticDatum")]
        public string GeodeticDatum;

        [JsonProperty("verticalDatum")]
        public string VerticalDatum;

        [JsonProperty("mapProjection")]
        public string MapProjection;

        [JsonProperty("mapZone")]
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