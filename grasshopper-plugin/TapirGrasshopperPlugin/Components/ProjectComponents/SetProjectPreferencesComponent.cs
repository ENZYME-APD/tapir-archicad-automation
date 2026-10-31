using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    // The pages of GetProjectPreferences and SetProjectPreferences, with the
    // fields each page has. A page travels as one JSON text, so the fields are
    // listed here to check the text before it is sent: a misspelled field would
    // otherwise only come back as a schema error of the whole command.
    internal static class ProjectPreferencesPages
    {
        // The preferenceTypes values of GetProjectPreferences, also the names
        // of the component parameters of the pages.
        public static readonly string[] TypeNames =
        {
            "WorkingUnits",
            "Dimensions",
            "CalculationUnits",
            "Zones"
        };

        // The keys of the pages in the commands' JSON, in the order of TypeNames.
        public static readonly string[] Keys =
        {
            "workingUnits",
            "dimensions",
            "calculationUnits",
            "zones"
        };

        private static Dictionary<string, object> Fields(
            params string[] names)
        {
            var fields = new Dictionary<string, object>();
            foreach (var name in names)
            {
                fields[name] = null;
            }

            return fields;
        }

        private static Dictionary<string, object> LengthDimFormat() =>
            Fields(
                "unit",
                "decimals",
                "roundInch",
                "accuracy",
                "showZeroWhole",
                "showZeroInch",
                "hideZeroDecimals");

        // The fields of each page: null for a value, a dictionary for an object
        // (or for the items of an array of objects).
        public static readonly Dictionary<string, object>[] PageFields =
        {
            new Dictionary<string, object>
            {
                ["length"] = Fields("unit", "decimals", "roundInch"),
                ["area"] = Fields("unit", "decimals"),
                ["volume"] = Fields("unit", "decimals"),
                ["angle"] = Fields("unit", "decimals", "accuracy")
            },
            new Dictionary<string, object>
            {
                ["dimensionStandardIndex"] = null,
                ["linear"] = LengthDimFormat(),
                ["angle"] = Fields("unit", "decimals", "hideZeroDecimals"),
                ["radial"] = LengthDimFormat(),
                ["level"] = LengthDimFormat(),
                ["elevation"] = LengthDimFormat(),
                ["doorWindow"] = LengthDimFormat(),
                ["parapet"] = LengthDimFormat(),
                ["area"] = Fields("unit", "decimals", "hideZeroDecimals")
            },
            new Dictionary<string, object>
            {
                ["length"] = Fields("unit", "accuracy", "decimals"),
                ["area"] = Fields("unit", "accuracy", "decimals"),
                ["volume"] = Fields("unit", "accuracy", "decimals"),
                ["angle"] = Fields("unit", "decimals", "accuracy")
            },
            new Dictionary<string, object>
            {
                ["lowCeilingAreaReduction"] = new Dictionary<string, object>
                {
                    ["activeHeightLimits"] = null,
                    ["heightLimits"] = Fields("height", "reductionPercent"),
                    ["reductionPercentAboveHeightLimits"] = null
                }
            }
        };

        // The first field of the token that the page does not have, or null.
        public static string FindUnknownField(
            JToken token,
            Dictionary<string, object> fields,
            string path)
        {
            if (token is JArray array)
            {
                foreach (var item in array)
                {
                    var unknown = FindUnknownField(
                        item,
                        fields,
                        path);
                    if (unknown != null)
                    {
                        return unknown;
                    }
                }

                return null;
            }

            if (!(token is JObject jObject))
            {
                return null;
            }

            foreach (var property in jObject.Properties())
            {
                if (!fields.TryGetValue(
                        property.Name,
                        out var subFields))
                {
                    return path + property.Name;
                }

                if (subFields is Dictionary<string, object> subFieldsOfObject)
                {
                    var unknown = FindUnknownField(
                        property.Value,
                        subFieldsOfObject,
                        path + property.Name + ".");
                    if (unknown != null)
                    {
                        return unknown;
                    }
                }
            }

            return null;
        }
    }

    public class SetProjectPreferencesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetProjectPreferences";

        public SetProjectPreferencesComponent()
            : base(
                "SetProjectPreferences",
                "Changes pages of the project preferences (Options > Project Preferences): the working units, " +
                "the dimensions, the calculation units and the zones. Each page is a JSON text, as " +
                "GetProjectPreferences returns it; only the given fields change, the rest of each page keeps its " +
                "value. In Teamwork the PreferencesDialog lockable object set has to be reserved first " +
                "(ReserveLockableObjectSet). Available from Archicad 27.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "WorkingUnits",
                "The fields of the Working Units page to change, as JSON: length (unit, decimals, roundInch), " +
                "area (unit, decimals), volume (unit, decimals) and angle (unit, decimals, accuracy).");

            InText(
                "Dimensions",
                "The fields of the Dimensions page to change, as JSON: dimensionStandardIndex; linear, radial, " +
                "level, elevation, doorWindow and parapet (unit, decimals, roundInch, accuracy, showZeroWhole, " +
                "showZeroInch, hideZeroDecimals); angle and area (unit, decimals, hideZeroDecimals).");

            InText(
                "CalculationUnits",
                "The fields of the Calculation Units page to change, as JSON: length, area and volume " +
                "(unit, accuracy, decimals) and angle (unit, decimals, accuracy).");

            InText(
                "Zones",
                "The fields of the Zones page to change, as JSON: lowCeilingAreaReduction (activeHeightLimits, " +
                "heightLimits: 1 to 3 items of height and reductionPercent changing the limit of the same index, " +
                "reductionPercentAboveHeightLimits).");

            SetOptionality(
                new[]
                {
                    0, 1, 2, 3
                });
        }

        protected override void AddOutputs()
        {
            for (var i = 0; i < ProjectPreferencesPages.TypeNames.Length; i++)
            {
                OutText(
                    ProjectPreferencesPages.TypeNames[i] + "Result",
                    $"Result of changing the {ProjectPreferencesPages.TypeNames[i]} page (null when it was not given).");
            }
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            for (var i = 0; i < ProjectPreferencesPages.Keys.Length; i++)
            {
                if (!da.TryGet(
                        i,
                        out string text) ||
                    string.IsNullOrWhiteSpace(text))
                {
                    continue;
                }

                JObject page;
                try
                {
                    page = JObject.Parse(text);
                }
                catch (JsonReaderException ex)
                {
                    this.AddError(
                        $"The input {ProjectPreferencesPages.TypeNames[i]} is not a JSON object: {ex.Message}");
                    return;
                }

                var unknownField = ProjectPreferencesPages.FindUnknownField(
                    page,
                    ProjectPreferencesPages.PageFields[i],
                    "");
                if (unknownField != null)
                {
                    this.AddError(
                        $"The input {ProjectPreferencesPages.TypeNames[i]} has an unknown field: {unknownField}.");
                    return;
                }

                parameters[ProjectPreferencesPages.Keys[i]] = page;
            }

            if (parameters.Count == 0)
            {
                this.AddError("Give at least one page to change.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                return;
            }

            for (var i = 0; i < ProjectPreferencesPages.Keys.Length; i++)
            {
                var result = response[ProjectPreferencesPages.Keys[i]];
                da.SetData(
                    i,
                    result == null
                        ? null
                        : ProjectComponentHelp.ExecutionResultMessage(result));
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetProjectPreferences;

        public override Guid ComponentGuid =>
            new Guid("136e36d3-e7da-4242-9ff7-27bb73c9db82");
    }
}
