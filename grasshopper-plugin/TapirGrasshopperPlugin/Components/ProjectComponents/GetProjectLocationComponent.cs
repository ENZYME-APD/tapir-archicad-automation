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

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Longitude",
                "Longitude",
                "Longitude",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "Latitude",
                "Latitude",
                "Latitude",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "Altitude",
                "Altitude",
                "Altitude",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "North",
                "North",
                "North",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "Eastings",
                "Eastings",
                "Eastings",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "Northings",
                "Northings",
                "Northings",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "Elevation",
                "Elevation",
                "Elevation",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "CrsName",
                "CrsName",
                "CrsName",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "Description",
                "Description",
                "Description",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "GeodeticDatum",
                "GeodeticDatum",
                "GeodeticDatum",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "VerticalDatum",
                "VerticalDatum",
                "VerticalDatum",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "MapProjection",
                "MapProjection",
                "MapProjection",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "MapZone",
                "MapZone",
                "MapZone",
                GH_ParamAccess.item);
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