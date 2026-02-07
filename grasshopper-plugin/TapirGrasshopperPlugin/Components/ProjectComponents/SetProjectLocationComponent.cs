using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetProjectLocationComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetGeoLocation";

        public SetProjectLocationComponent()
            : base(
                "SetProjectLocation",
                "Set Geo Location of the currently active project.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(nameof(Location.Longitude));
            InText(nameof(Location.Latitude));
            InText(nameof(Location.Altitude));
            InText(nameof(Location.North));
            InText(nameof(SurveyPointPosition.Eastings));
            InText(nameof(SurveyPointPosition.Northings));
            InText(nameof(SurveyPointPosition.Elevation));
            InText(nameof(GeoReferencingParameters.CrsName));
            InText(nameof(GeoReferencingParameters.Description));
            InText(nameof(GeoReferencingParameters.GeodeticDatum));
            InText(nameof(GeoReferencingParameters.VerticalDatum));
            InText(nameof(GeoReferencingParameters.MapProjection));
            InText(nameof(GeoReferencingParameters.MapZone));

            SetOptionality(
                new[]
                {
                    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
                });
        }

        private void ParseInputNumber(IGH_DataAccess da, int i, ref double? value)
        {
            da.TryGet(
                    i,
                    out string input);
            if (input != null && double.TryParse(input, out double d))
            {
                value = Math.Round(d, 6, MidpointRounding.AwayFromZero);
            } else
            {
                value = null;
            }
        }

        private void ParseInputString(IGH_DataAccess da, int i, ref string value)
        {
            da.TryGet(
                    i,
                    out string input);
            value = input;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var projectLocation = new ProjectLocation()
            {
                    Loc = new Location(),
                    Survey = new SurveyPoint()
                    {
                        Position = new SurveyPointPosition(),
                        GeoReferencingParams = new GeoReferencingParameters()
                    }
            };

            ParseInputNumber(da, 0, ref projectLocation.Loc.Longitude);
            ParseInputNumber(da, 1, ref projectLocation.Loc.Latitude);
            ParseInputNumber(da, 2, ref projectLocation.Loc.Altitude);
            ParseInputNumber(da, 3, ref projectLocation.Loc.North);
            ParseInputNumber(da, 4, ref projectLocation.Survey.Position.Eastings);
            ParseInputNumber(da, 5, ref projectLocation.Survey.Position.Northings);
            ParseInputNumber(da, 6, ref projectLocation.Survey.Position.Elevation);
            ParseInputString(da, 7, ref projectLocation.Survey.GeoReferencingParams.CrsName);
            ParseInputString(da, 8, ref projectLocation.Survey.GeoReferencingParams.Description);
            ParseInputString(da, 9, ref projectLocation.Survey.GeoReferencingParams.GeodeticDatum);
            ParseInputString(da, 10, ref projectLocation.Survey.GeoReferencingParams.VerticalDatum);
            ParseInputString(da, 11, ref projectLocation.Survey.GeoReferencingParams.MapProjection);
            ParseInputString(da, 12, ref projectLocation.Survey.GeoReferencingParams.MapZone);

            SetCadValues(
                CommandName,
                projectLocation,
                ToAddOn);
        }

        public override Guid ComponentGuid =>
            new Guid("6b29d0b8-8075-4340-bb7c-cfcde02c9e0e");
    }
}