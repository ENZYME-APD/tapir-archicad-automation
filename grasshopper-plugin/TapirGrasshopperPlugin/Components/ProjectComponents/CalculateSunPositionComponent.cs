using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class CalculateSunPositionComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "CalculateSunPosition";

        public CalculateSunPositionComponent()
            : base(
                "CalculateSunPosition",
                "Calculates the sun position (azimuth and altitude in degrees) for a date and time, at the project " +
                "location unless another one is given, without changing the project. Every unconnected input keeps " +
                "the project's value.",
                GroupNames.Project)
        {
        }

        // The location fields (ProjectLocationUpdate): parameter name, JSON key
        // and description.
        private static readonly string[,] LocationFields =
        {
            { "Longitude", "longitude", "Longitude in degrees." },
            { "Latitude", "latitude", "Latitude in degrees." },
            { "Altitude", "altitude", "Altitude in meters." },
            { "North", "north", "North direction in radians." }
        };

        protected override void AddInputs()
        {
            for (var i = 0; i < SunDateTimeFields.Count; i++)
            {
                this.AddOptionalInteger(
                    SunDateTimeFields.Name(i),
                    $"The {SunDateTimeFields.Key(i)} of the date and time. Unconnected keeps the project's value.");
            }

            this.AddOptionalBoolean(
                SunDateTimeFields.DaylightSavingName,
                "True when the time is daylight saving (summer) time. Unconnected keeps the project's value.");

            for (var i = 0; i < LocationFields.GetLength(0); i++)
            {
                this.AddOptionalNumber(
                    LocationFields[i, 0],
                    LocationFields[i, 2] + " Unconnected keeps the project location's value.");
            }
        }

        protected override void AddOutputs()
        {
            this.AddNumberOutput(
                "SunAzimuth",
                "The sun azimuth in degrees: the direction of the sun in the project's XY plane, as the 3D " +
                "Projection Settings show it.");

            this.AddNumberOutput(
                "SunAltitude",
                "The sun altitude in degrees: the elevation of the sun above the horizon, as the 3D Projection " +
                "Settings show it. Negative while the sun is below the horizon.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var dateTime = new JObject();
            for (var i = 0; i < SunDateTimeFields.Count; i++)
            {
                da.TryAddOptional<int>(
                    i,
                    dateTime,
                    SunDateTimeFields.Key(i));
            }

            var index = SunDateTimeFields.Count;
            da.TryAddOptional<bool>(
                index++,
                dateTime,
                SunDateTimeFields.DaylightSavingKey);

            var location = new JObject();
            for (var i = 0; i < LocationFields.GetLength(0); i++)
            {
                da.TryAddOptional<double>(
                    index++,
                    location,
                    LocationFields[i, 1]);
            }

            var parameters = new JObject();
            if (dateTime.Count > 0)
            {
                parameters["dateTime"] = dateTime;
            }

            if (location.Count > 0)
            {
                parameters["location"] = location;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                return;
            }

            da.SetData(
                0,
                response.Value<double?>("azimuth"));
            da.SetData(
                1,
                response.Value<double?>("altitude"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CalculateSunPosition;

        public override Guid ComponentGuid =>
            new Guid("a8f5db9e-fa5d-4717-a143-5cffff9447b9");
    }
}
