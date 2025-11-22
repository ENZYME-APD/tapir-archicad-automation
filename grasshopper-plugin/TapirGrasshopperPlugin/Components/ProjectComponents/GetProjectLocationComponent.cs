using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
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

    public class GetProjectLocationComponent : ArchicadAccessorComponent
    {
        public static string Doc =>
            "Get Geo Location of the currently active project.";

        public override string CommandName => "GetGeoLocation";

        public GetProjectLocationComponent()
            : base(
                "Project Location",
                "ProjectLocation",
                Doc,
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "Longitude",
                "Longitude");

            OutText(
                "Latitude",
                "Latitude");

            OutText(
                "Altitude",
                "Altitude");

            OutText(
                "North",
                "North");

            OutText(
                "Eastings",
                "Eastings");

            OutText(
                "Northings",
                "Northings");

            OutText(
                "Elevation",
                "Elevation");

            OutText(
                "CrsName",
                "CrsName");

            OutText(
                "Description",
                "Description");

            OutText(
                "GeodeticDatum",
                "GeodeticDatum");

            OutText(
                "VerticalDatum",
                "VerticalDatum");

            OutText(
                "MapProjection",
                "MapProjection");

            OutText(
                "MapZone",
                "MapZone");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!GetConvertedResponse(
                    CommandName,
                    out ProjectLocation response))
            {
                return;
            }

            da.SetData(
                0,
                response.Loc.Longitude);
            da.SetData(
                1,
                response.Loc.Latitude);
            da.SetData(
                2,
                response.Loc.Altitude);
            da.SetData(
                3,
                response.Loc.North);
            da.SetData(
                4,
                response.Survey.Position.Eastings);
            da.SetData(
                5,
                response.Survey.Position.Northings);
            da.SetData(
                6,
                response.Survey.Position.Elevation);
            da.SetData(
                7,
                response.Survey.GeoReferencingParams.CrsName);
            da.SetData(
                8,
                response.Survey.GeoReferencingParams.Description);
            da.SetData(
                9,
                response.Survey.GeoReferencingParams.GeodeticDatum);
            da.SetData(
                10,
                response.Survey.GeoReferencingParams.VerticalDatum);
            da.SetData(
                11,
                response.Survey.GeoReferencingParams.MapProjection);
            da.SetData(
                12,
                response.Survey.GeoReferencingParams.MapZone);
        }

        public override Guid ComponentGuid =>
            new Guid("57989cda-f956-4b2a-9ce3-a7b4503ea158");
    }
}