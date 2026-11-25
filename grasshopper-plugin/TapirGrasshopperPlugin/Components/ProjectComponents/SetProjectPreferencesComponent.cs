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
            "Zones",
            "ReferenceLevels",
            "FloorPlanCutPlane",
            "Legacy",
            "ImagingAndCalculation",
            "Layouts",
            "DataSafety",
            "CalculationRules"
        };

        // The keys of the pages in the commands' JSON, in the order of TypeNames.
        public static readonly string[] Keys =
        {
            "workingUnits",
            "dimensions",
            "calculationUnits",
            "zones",
            "referenceLevels",
            "floorPlanCutPlane",
            "legacy",
            "imagingAndCalculation",
            "layouts",
            "dataSafety",
            "calculationRules"
        };

        // The indices (into TypeNames and Keys) of the pages SetProjectPreferences
        // writes, in the order of its inputs and outputs: Data Safety is read only.
        // New pages are appended, so saved definitions keep their parameters.
        public static readonly int[] SettableIndices =
        {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 10
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

        // The items of a list of attributes: { "attributeId": { "guid": ... } }.
        private static Dictionary<string, object> AttributeIdArrayItem() =>
            new Dictionary<string, object>
            {
                ["attributeId"] = Fields("guid")
            };

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
            },
            new Dictionary<string, object>
            {
                ["level1"] = Fields("name", "elevation"),
                ["level2"] = Fields("name", "elevation")
            },
            new Dictionary<string, object>
            {
                ["cutPlaneHeight"] = null,
                ["topLevel"] = Fields("relativeStory", "offset"),
                ["bottomLevel"] = Fields("relativeStory", "offset"),
                ["absoluteDisplayLimit"] = null
            },
            new Dictionary<string, object>
            {
                ["columnIntersectionPriority"] = null,
                ["aboveHomeStoryLineTypeId"] = Fields("guid"),
                ["belowHomeStoryLineTypeId"] = Fields("guid"),
                ["roofContourDisplay"] = null,
                ["useLegacyIntersections"] = null,
                ["hideZonesOnSections"] = null
            },
            Fields(
                "autoRebuild3D",
                "activate3DOnChange",
                "keepZoomedSection",
                "showProgressWindow",
                "interruptOnError",
                "progressSounds",
                "generate3DReport",
                "reportLevel",
                "selectedElementsListing"),
            new Dictionary<string, object>
            {
                ["masterItemsColor"] = Fields("red", "green", "blue"),
                ["useOwnMasterItemsColor"] = null,
                ["adjustDrawingFrameToViewZoom"] = null
            },
            Fields("temporaryFolder"),
            new Dictionary<string, object>
            {
                ["elementRules"] = Fields("propertyType", "holeLimit", "elementTypes"),
                ["wallInsulationFills"] = AttributeIdArrayItem(),
                ["wallAirFills"] = AttributeIdArrayItem(),
                ["roofInsulationFills"] = AttributeIdArrayItem(),
                ["shellInsulationFills"] = AttributeIdArrayItem()
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
                "the dimensions, the calculation units, the zones, the reference levels, the legacy settings, " +
                "the layouts and the calculation rules, and the Floor Plan Cut Plane Settings, as well as the " +
                "Imaging and Calculation page of the Work Environment. Each page is a JSON text, as " +
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

            InText(
                "ReferenceLevels",
                "The fields of the Reference Levels page to change, as JSON: level1 and level2 (name, " +
                "elevation from Project Zero).");

            InText(
                "FloorPlanCutPlane",
                "The fields of the Floor Plan Cut Plane Settings to change, as JSON: cutPlaneHeight, topLevel and " +
                "bottomLevel (relativeStory, offset), absoluteDisplayLimit.");

            InText(
                "Legacy",
                "The fields of the Legacy page to change, as JSON: columnIntersectionPriority, " +
                "aboveHomeStoryLineTypeId and belowHomeStoryLineTypeId (guid of a line type), roofContourDisplay " +
                "(AllVisibleContours or OnlyTheTopSurface), useLegacyIntersections, hideZonesOnSections.");

            InText(
                "ImagingAndCalculation",
                "The fields of the Imaging and Calculation page to change, as JSON: autoRebuild3D, " +
                "activate3DOnChange, keepZoomedSection, showProgressWindow, interruptOnError, progressSounds, " +
                "generate3DReport, reportLevel (No, Brief, Short, Detailed or Full), selectedElementsListing " +
                "(ShowAlert, ListAll or UseFilters). This page belongs to the Work Environment, so it changes " +
                "the settings of Archicad, not of the project.");

            InText(
                "Layouts",
                "The fields of the Layouts page to change, as JSON: masterItemsColor (red, green, blue from 0 " +
                "to 1), useOwnMasterItemsColor, adjustDrawingFrameToViewZoom.");

            InText(
                "CalculationRules",
                "The fields of the Calculation Rules page to change, as JSON: elementRules (items of " +
                "propertyType: Volume, Surface or Length, holeLimit and elementTypes: Wall, Slab, Roof, Shell, " +
                "Beam), wallInsulationFills, wallAirFills, roofInsulationFills and shellInsulationFills (items of " +
                "attributeId with the guid of a fill). Each list given replaces the whole list.");

            SetOptionality(
                new[]
                {
                    0, 1, 2, 3, 4, 5, 6, 7, 8, 9
                });
        }

        protected override void AddOutputs()
        {
            foreach (var i in ProjectPreferencesPages.SettableIndices)
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
            for (var p = 0; p < ProjectPreferencesPages.SettableIndices.Length; p++)
            {
                var i = ProjectPreferencesPages.SettableIndices[p];
                if (!da.TryGet(
                        p,
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

            for (var p = 0; p < ProjectPreferencesPages.SettableIndices.Length; p++)
            {
                var result = response[ProjectPreferencesPages.Keys[ProjectPreferencesPages.SettableIndices[p]]];
                da.SetData(
                    p,
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
