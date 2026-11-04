using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetPlaceSettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetPlaceSettings";

        private enum FieldKind
        {
            Number,
            Integer,
            Boolean
        }

        // The fields of API_PlaceInfo the command takes, in the order of the
        // inputs and of the first outputs: the Grasshopper name, the JSON key,
        // the kind of the value and the description.
        private static readonly (string Name, string Key, FieldKind Kind, string Description)[] Fields =
        {
            ("Longitude", "longitude", FieldKind.Number, "Longitude in degrees."),
            ("Latitude", "latitude", FieldKind.Number, "Latitude in degrees."),
            ("Altitude", "altitude", FieldKind.Number, "Altitude in meters."),
            ("North", "north", FieldKind.Number, "The north direction in radians."),
            ("Year", "year", FieldKind.Integer, "The year of the date, 0 to 65535."),
            ("Month", "month", FieldKind.Integer, "The month of the date, 1 to 12."),
            ("Day", "day", FieldKind.Integer, "The day of the date, 1 to 31."),
            ("Hour", "hour", FieldKind.Integer, "The hour of the time, 0 to 23."),
            ("Minute", "minute", FieldKind.Integer, "The minute of the time, 0 to 59."),
            ("Second", "second", FieldKind.Integer, "The second of the time, 0 to 59."),
            ("SummerTime", "sumTime", FieldKind.Boolean, "Whether summer time (+1 hour) is on."),
            ("TimeZoneInMinutes", "timeZoneInMinutes", FieldKind.Integer, "The time zone, in minutes from Greenwich."),
            ("TimeZoneOffset", "timeZoneOffset", FieldKind.Integer, "Tells apart time zones with the same TimeZoneInMinutes.")
        };

        public SetPlaceSettingsComponent()
            : base(
                "SetPlaceSettings",
                "Changes the place settings of the project: location (longitude, latitude, altitude, north), " +
                "date, time, summer time and time zone. Only the connected inputs change; with none connected " +
                "the current settings are returned. Returns the settings read back, with the sun position " +
                "Archicad calculated for them. Available from Archicad 27.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            // Inputs without a default value: an unconnected one is not sent,
            // so that setting stays unchanged.
            for (var i = 0; i < Fields.Length; i++)
            {
                var field = Fields[i];
                var description = "Optional. " + field.Description + " Unconnected means unchanged.";
                switch (field.Kind)
                {
                    case FieldKind.Number:
                        inManager.AddNumberParameter(
                            field.Name,
                            field.Name,
                            description.WithTypeName("numberItem"),
                            GH_ParamAccess.item);
                        break;
                    case FieldKind.Integer:
                        inManager.AddIntegerParameter(
                            field.Name,
                            field.Name,
                            description.WithTypeName("integerItem"),
                            GH_ParamAccess.item);
                        break;
                    default:
                        inManager.AddBooleanParameter(
                            field.Name,
                            field.Name,
                            description.WithTypeName("booleanItem"),
                            GH_ParamAccess.item);
                        break;
                }
                SetOptionality(i);
            }
        }

        protected override void AddOutputs()
        {
            foreach (var field in Fields)
            {
                var description = field.Description + " Read back after the change.";
                switch (field.Kind)
                {
                    case FieldKind.Number:
                        outManager.AddNumberParameter(
                            field.Name,
                            field.Name,
                            description.WithTypeName("numberItem"),
                            GH_ParamAccess.item);
                        break;
                    case FieldKind.Integer:
                        OutInteger(
                            field.Name,
                            description);
                        break;
                    default:
                        OutBoolean(
                            field.Name,
                            description);
                        break;
                }
            }

            outManager.AddNumberParameter(
                "SunAngXY",
                "SunAngXY",
                "The horizontal angle of the sun Archicad calculated for the place, date and time, in radians."
                    .WithTypeName("numberItem"),
                GH_ParamAccess.item);

            outManager.AddNumberParameter(
                "SunAngZ",
                "SunAngZ",
                "The altitude angle of the sun Archicad calculated for the place, date and time, in radians."
                    .WithTypeName("numberItem"),
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var placeInfo = new JObject();

            for (var i = 0; i < Fields.Length; i++)
            {
                var field = Fields[i];
                switch (field.Kind)
                {
                    case FieldKind.Number:
                        if (da.TryGet(
                                i,
                                out double number))
                        {
                            placeInfo[field.Key] = number;
                        }
                        break;
                    case FieldKind.Integer:
                        if (da.TryGet(
                                i,
                                out int integer))
                        {
                            placeInfo[field.Key] = integer;
                        }
                        break;
                    default:
                        if (da.TryGet(
                                i,
                                out bool boolean))
                        {
                            placeInfo[field.Key] = boolean;
                        }
                        break;
                }
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["placeInfo"] = placeInfo },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var result = response["placeInfo"];
            if (result == null || result.Type != JTokenType.Object)
            {
                this.AddError("The response has no place settings.");
                return;
            }

            for (var i = 0; i < Fields.Length; i++)
            {
                var field = Fields[i];
                switch (field.Kind)
                {
                    case FieldKind.Number:
                        da.SetData(
                            i,
                            result.Value<double?>(field.Key));
                        break;
                    case FieldKind.Integer:
                        da.SetData(
                            i,
                            result.Value<int?>(field.Key));
                        break;
                    default:
                        da.SetData(
                            i,
                            result.Value<bool?>(field.Key));
                        break;
                }
            }

            da.SetData(
                Fields.Length,
                result.Value<double?>("sunAngXY"));

            da.SetData(
                Fields.Length + 1,
                result.Value<double?>("sunAngZ"));
        }

        // Shares the icon of SetProjectLocation until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetProjectLocation;

        public override Guid ComponentGuid =>
            new Guid("4250ecf8-788b-4486-8149-687b84eeaf93");
    }
}
