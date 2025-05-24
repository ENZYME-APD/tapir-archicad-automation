using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class Location
    {
        [JsonProperty ("longitude")]
        public double Longitude;

        [JsonProperty ("latitude")]
        public double Latitude;

        [JsonProperty ("altitude")]
        public double Altitude;

        [JsonProperty ("north")]
        public double North;
    }

    public class SurveyPointPosition
    {
        [JsonProperty ("eastings")]
        public double Eastings;

        [JsonProperty ("northings")]
        public double Northings;

        [JsonProperty ("elevation")]
        public double Elevation;
    }

    public class GeoReferencingParameters
    {
        [JsonProperty ("crsName")]
        public string CrsName;

        [JsonProperty ("description")]
        public string Description;

        [JsonProperty ("geodeticDatum")]
        public string GeodeticDatum;

        [JsonProperty ("verticalDatum")]
        public string VerticalDatum;

        [JsonProperty ("mapProjection")]
        public string MapProjection;

        [JsonProperty ("mapZone")]
        public string MapZone;
    }

    public class SurveyPoint
    {
        [JsonProperty ("position")]
        public SurveyPointPosition Position;

        [JsonProperty ("geoReferencingParameters")]
        public GeoReferencingParameters GeoReferencingParams;
    }

    public class ProjectLocation
    {
        [JsonProperty ("projectLocation")]
        public Location Loc;

        [JsonProperty ("surveyPoint")]
        public SurveyPoint Survey;
    }

    public class GetProjectLocationComponent : ArchicadAccessorComponent
    {
        public GetProjectLocationComponent ()
            : base (
                "Project Location",
                "ProjectLocation",
                "Get Geo Location of the currently active project.",
                "Project"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter ("Longitude", "Longitude", "Longitude", GH_ParamAccess.item);
            pManager.AddTextParameter ("Latitude", "Latitude", "Latitude", GH_ParamAccess.item);
            pManager.AddTextParameter ("Altitude", "Altitude", "Altitude", GH_ParamAccess.item);
            pManager.AddTextParameter ("North", "North", "North", GH_ParamAccess.item);
            pManager.AddTextParameter ("Eastings", "Eastings", "Eastings", GH_ParamAccess.item);
            pManager.AddTextParameter ("Northings", "Northings", "Northings", GH_ParamAccess.item);
            pManager.AddTextParameter ("Elevation", "Elevation", "Elevation", GH_ParamAccess.item);
            pManager.AddTextParameter ("CrsName", "CrsName", "CrsName", GH_ParamAccess.item);
            pManager.AddTextParameter ("Description", "Description", "Description", GH_ParamAccess.item);
            pManager.AddTextParameter ("GeodeticDatum", "GeodeticDatum", "GeodeticDatum", GH_ParamAccess.item);
            pManager.AddTextParameter ("VerticalDatum", "VerticalDatum", "VerticalDatum", GH_ParamAccess.item);
            pManager.AddTextParameter ("MapProjection", "MapProjection", "MapProjection", GH_ParamAccess.item);
            pManager.AddTextParameter ("MapZone", "MapZone", "MapZone", GH_ParamAccess.item);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetGeoLocation", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            ProjectLocation projectLocation = response.Result.ToObject<ProjectLocation> ();
            DA.SetData (0, projectLocation.Loc.Longitude);
            DA.SetData (1, projectLocation.Loc.Latitude);
            DA.SetData (2, projectLocation.Loc.Altitude);
            DA.SetData (3, projectLocation.Loc.North);
            DA.SetData (4, projectLocation.Survey.Position.Eastings);
            DA.SetData (5, projectLocation.Survey.Position.Northings);
            DA.SetData (6, projectLocation.Survey.Position.Elevation);
            DA.SetData (7, projectLocation.Survey.GeoReferencingParams.CrsName);
            DA.SetData (8, projectLocation.Survey.GeoReferencingParams.Description);
            DA.SetData (9, projectLocation.Survey.GeoReferencingParams.GeodeticDatum);
            DA.SetData (10, projectLocation.Survey.GeoReferencingParams.VerticalDatum);
            DA.SetData (11, projectLocation.Survey.GeoReferencingParams.MapProjection);
            DA.SetData (12, projectLocation.Survey.GeoReferencingParams.MapZone);
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ProjectLocation;

        public override Guid ComponentGuid => new Guid ("57989cda-f956-4b2a-9ce3-a7b4503ea158");
    }
}
