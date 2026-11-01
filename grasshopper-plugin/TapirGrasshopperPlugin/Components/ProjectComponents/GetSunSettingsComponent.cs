using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    // The date and time fields of the sun commands (SunDateTime): the
    // parameter name of the components and the key of the commands' JSON.
    internal static class SunDateTimeFields
    {
        public static readonly string[,] Fields =
        {
            { "Year", "year" },
            { "Month", "month" },
            { "Day", "day" },
            { "Hour", "hour" },
            { "Minute", "minute" },
            { "Second", "second" }
        };

        public const string DaylightSavingName = "DaylightSaving";
        public const string DaylightSavingKey = "daylightSaving";

        public static int Count => Fields.GetLength(0);

        public static string Name(
            int index) => Fields[index, 0];

        public static string Key(
            int index) => Fields[index, 1];
    }

    public class GetSunSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetSunSettings";

        public GetSunSettingsComponent()
            : base(
                "GetSunSettings",
                "Gets the project's date and time (Options > Project Preferences > Location Settings), the sun " +
                "position Archicad calculates for them at the project location, and the sun and ambient light settings.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            for (var i = 0; i < SunDateTimeFields.Count; i++)
            {
                OutInteger(
                    SunDateTimeFields.Name(i),
                    $"The {SunDateTimeFields.Key(i)} of the project's date and time.");
            }

            OutBoolean(
                SunDateTimeFields.DaylightSavingName,
                "True when the project's time is daylight saving (summer) time.");

            OutInteger(
                "TimeZoneInMinutes",
                "The time zone of the project location in minutes, as Archicad stores it.");

            OutInteger(
                "TimeZoneOffset",
                "The time zone offset of the project location in minutes, as Archicad stores it.");

            this.AddNumberOutput(
                "SunAzimuth",
                "The sun azimuth in degrees for the project's date, time and location: the direction of the sun " +
                "in the project's XY plane, as the 3D Projection Settings show it.");

            this.AddNumberOutput(
                "SunAltitude",
                "The sun altitude in degrees for the project's date, time and location: the elevation of the sun " +
                "above the horizon, as the 3D Projection Settings show it.");

            this.AddColorOutput(
                "SunColor",
                "The color of the sun light (null when Archicad cannot report the light settings).");

            OutInteger(
                "SunIntensity",
                "The intensity of the sun light in percent.");

            this.AddColorOutput(
                "AmbientColor",
                "The color of the ambient light.");

            OutInteger(
                "AmbientIntensity",
                "The intensity of the ambient light in percent.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var dateTime = response["dateTime"];
            for (var i = 0; i < SunDateTimeFields.Count; i++)
            {
                da.SetData(
                    i,
                    dateTime?.Value<int?>(SunDateTimeFields.Key(i)));
            }

            var index = SunDateTimeFields.Count;
            da.SetData(
                index++,
                dateTime?.Value<bool?>(SunDateTimeFields.DaylightSavingKey));
            da.SetData(
                index++,
                response.Value<int?>("timeZoneInMinutes"));
            da.SetData(
                index++,
                response.Value<int?>("timeZoneOffset"));
            da.SetData(
                index++,
                response["sunPosition"]?.Value<double?>("azimuth"));
            da.SetData(
                index++,
                response["sunPosition"]?.Value<double?>("altitude"));

            var sunLight = response["sunLight"];
            da.SetData(
                index++,
                JsonOutputHelp.ColorOf(sunLight?["sunColor"]));
            da.SetData(
                index++,
                sunLight?.Value<int?>("sunIntensity"));
            da.SetData(
                index++,
                JsonOutputHelp.ColorOf(sunLight?["ambientColor"]));
            da.SetData(
                index,
                sunLight?.Value<int?>("ambientIntensity"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetSunSettings;

        public override Guid ComponentGuid =>
            new Guid("64e02813-d160-42f3-913a-1d788616f3d2");
    }
}
