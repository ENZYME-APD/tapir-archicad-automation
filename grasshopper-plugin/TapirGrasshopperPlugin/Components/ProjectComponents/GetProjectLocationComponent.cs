using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetProjectLocationComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGeoLocation";

        public GetProjectLocationComponent()
            : base(
                "ProjectLocation",
                "Get Geo Location of the currently active project.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(Location.Longitude),
                "");

            OutText(
                nameof(Location.Latitude),
                "");

            OutText(
                nameof(Location.Altitude),
                "");

            OutText(
                nameof(Location.North),
                "");

            OutText(
                nameof(SurveyPointPosition.Eastings),
                "");

            OutText(
                nameof(SurveyPointPosition.Northings),
                "");

            OutText(
                "Elevation",
                "");

            OutText(
                nameof(GeoReferencingParameters.CrsName),
                "");

            OutText(
                nameof(GeoReferencingParameters.Description),
                "");

            OutText(
                nameof(GeoReferencingParameters.GeodeticDatum),
                "");

            OutText(
                nameof(GeoReferencingParameters.VerticalDatum),
                "");

            OutText(
                nameof(GeoReferencingParameters.MapProjection),
                "");

            OutText(
                nameof(GeoReferencingParameters.MapZone),
                "");
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