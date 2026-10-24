using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    // Shared code of the GetProjectPreferences and SetProjectPreferences
    // components. The commands answer and take the pages of Options > Project
    // Preferences as nested JSON objects (pages by name, each a set of named
    // objects with their fields). The components expose the pages that are
    // tables of rows with the same fields - the quantities of the Working
    // Units and Calculation Units pages and the dimension types of the
    // Dimensions page - as one list per column, the Zones page as plain
    // values, and the whole structure as one JSON text, which covers the
    // remaining fields (round inch, zero handling, the dimension standard, the
    // integer angle accuracy) and restores saved preferences in one go.
    internal static class ProjectPreferencesHelp
    {
        public const string JsonOutputDescription =
            "JSON object of every page the add-on answered, pages by name: workingUnits, " +
            "calculationUnits, dimensions and zones. SetProjectPreferences takes it back as it is, " +
            "so it restores saved preferences. The fields the other outputs do not cover (round inch, " +
            "zero handling, the dimension standard, the integer angle accuracy) are in it too.";

        public const string JsonInputDescription =
            "Optional. JSON object of the pages to change, pages by name (workingUnits, calculationUnits, " +
            "dimensions, zones) in the format GetProjectPreferences outputs in JsonPreferences: that " +
            "output restores saved preferences, and a partial object like " +
            "{\"workingUnits\": {\"length\": {\"roundInch\": 16}}} changes a field the other inputs " +
            "do not cover. The other inputs are applied on top of it.";

        public const string UnitNames =
            "Length units: Meter, Decimeter, Centimeter, Millimeter, FootFracInch, FootDecInch, DecFoot, " +
            "FracInch, DecInch. Area units: SquareMeter, SquareCentimeter, SquareMillimeter, SquareFoot, " +
            "SquareInch. Volume units: CubicMeter, Liter, CubicCentimeter, CubicMillimeter, CubicFoot, " +
            "CubicInch, CubicYard, Gallon. Angle units: DecimalDegree, DegreeMinSec, Grad, Radian, Surveyors.";

        public const string AccuracyNames =
            "Off, ShowSmall5, ShowSmall25, ShowSmall1, ShowSmall01 or InchCaseFractions";

        // One row of a table page: a quantity of the units pages or a
        // dimension type of the Dimensions page.
        public sealed class Row
        {
            // The name in Grasshopper, e.g. DoorWindow.
            public string Name { get; }

            // The field of the page object, e.g. doorWindow.
            public string Key { get; }

            // Whether the row has an extra accuracy name (see AccuracyNames).
            // The angle rows have an integer accuracy instead, and the area
            // dimensions have none; both are left to the JSON input.
            public bool HasAccuracy { get; }

            public Row(
                string name,
                string key,
                bool hasAccuracy)
            {
                Name = name;
                Key = key;
                HasAccuracy = hasAccuracy;
            }
        }

        // A page that is a table of rows with the fields unit, decimals and,
        // where the page has them, accuracy and hideZeroDecimals.
        public sealed class Table
        {
            public string PageKey { get; }
            public string PageName { get; }
            public string RowWord { get; }
            public IReadOnlyList<Row> Rows { get; }
            public string RowsName { get; }
            public string UnitsName { get; }
            public string DecimalsName { get; }

            // Null when the page has no extra accuracy column.
            public string AccuraciesName { get; }

            // Null when the page has no hideZeroDecimals column.
            public string HideZeroDecimalsName { get; }

            public Table(
                string pageKey,
                string pageName,
                string rowWord,
                Row[] rows,
                string rowsName,
                string unitsName,
                string decimalsName,
                string accuraciesName,
                string hideZeroDecimalsName)
            {
                PageKey = pageKey;
                PageName = pageName;
                RowWord = rowWord;
                Rows = rows;
                RowsName = rowsName;
                UnitsName = unitsName;
                DecimalsName = decimalsName;
                AccuraciesName = accuraciesName;
                HideZeroDecimalsName = hideZeroDecimalsName;
            }

            // The number of inputs or outputs the table has on a component.
            public int ParamCount =>
                3 +
                (AccuraciesName == null ? 0 : 1) +
                (HideZeroDecimalsName == null ? 0 : 1);

            private string RowNames =>
                string.Join(", ", Rows.Select(row => row.Name));

            public void AddOutputs(
                Component component)
            {
                component.OutTexts(
                    RowsName,
                    $"The {RowWord}s of the {PageName} page: {RowNames}. Empty when the add-on does not know the page.");

                component.OutTexts(
                    UnitsName,
                    $"Unit of each {RowWord}.");

                component.OutIntegers(
                    DecimalsName,
                    $"Number of decimals of each {RowWord}.");

                if (AccuraciesName != null)
                {
                    component.OutTexts(
                        AccuraciesName,
                        $"Extra accuracy of each {RowWord} ({AccuracyNames}); empty for the {RowWord}s that have none.");
                }

                if (HideZeroDecimalsName != null)
                {
                    component.OutBooleans(
                        HideZeroDecimalsName,
                        $"Whether the zero decimals at the end of the values of each {RowWord} are hidden.");
                }
            }

            // Hands the rows of the page out, from the output at firstIndex
            // on; empty lists when the page is not in the response.
            public void SetOutputs(
                IGH_DataAccess da,
                JToken page,
                int firstIndex)
            {
                var names = new List<string>();
                var units = new List<object>();
                var decimals = new List<object>();
                var accuracies = new List<object>();
                var hideZeroDecimals = new List<object>();

                if (page is JObject)
                {
                    foreach (var row in Rows)
                    {
                        var fields = page[row.Key];
                        names.Add(row.Name);
                        units.Add(JsonOutputHelp.Scalar(fields, "unit"));
                        decimals.Add(JsonOutputHelp.Scalar(fields, "decimals"));
                        accuracies.Add(
                            row.HasAccuracy
                                ? JsonOutputHelp.Scalar(fields, "accuracy")
                                : null);
                        hideZeroDecimals.Add(JsonOutputHelp.Scalar(fields, "hideZeroDecimals"));
                    }
                }

                var index = firstIndex;
                da.SetDataList(index++, names);
                da.SetDataList(index++, units);
                da.SetDataList(index++, decimals);
                if (AccuraciesName != null)
                {
                    da.SetDataList(index++, accuracies);
                }
                if (HideZeroDecimalsName != null)
                {
                    da.SetDataList(index++, hideZeroDecimals);
                }
            }

            public void AddInputs(
                Component component)
            {
                component.InTexts(
                    RowsName,
                    $"The {RowWord}s of the {PageName} page to change: {RowNames}. " +
                    "The other inputs of the page give their new values: one value for all or one per " +
                    $"{RowWord}; a field with no value keeps its setting.");

                component.InTexts(
                    UnitsName,
                    $"New unit of each given {RowWord} (an empty text keeps the unit). " + UnitNames);

                component.InIntegers(
                    DecimalsName,
                    $"New number of decimals of each given {RowWord}.");

                if (AccuraciesName != null)
                {
                    component.InTexts(
                        AccuraciesName,
                        $"New extra accuracy of each given {RowWord}: {AccuracyNames} (an empty text keeps it). " +
                        $"A single value is applied to the given {RowWord}s that have an extra accuracy.");
                }

                if (HideZeroDecimalsName != null)
                {
                    component.InBooleans(
                        HideZeroDecimalsName,
                        $"Whether to hide the zero decimals at the end of the values of each given {RowWord}.");
                }
            }

            // Reads the inputs of the page from firstIndex on and puts the
            // given fields into the page object of parameters. Returns false
            // after reporting an error on the component.
            public bool TryApplyInputs(
                Component component,
                IGH_DataAccess da,
                int firstIndex,
                JObject parameters)
            {
                var index = firstIndex;
                var rowNames = da.GetOptionals<string>(index++);
                var units = da.GetOptionals<string>(index++);
                var decimals = da.GetOptionals<int>(index++);
                var accuracies = AccuraciesName == null
                    ? new List<string>()
                    : da.GetOptionals<string>(index++);
                var hideZeroDecimals = HideZeroDecimalsName == null
                    ? new List<bool>()
                    : da.GetOptionals<bool>(index++);

                if (rowNames.Count == 0)
                {
                    string given = null;
                    if (units.Count > 0)
                    {
                        given = UnitsName;
                    }
                    else if (decimals.Count > 0)
                    {
                        given = DecimalsName;
                    }
                    else if (accuracies.Count > 0)
                    {
                        given = AccuraciesName;
                    }
                    else if (hideZeroDecimals.Count > 0)
                    {
                        given = HideZeroDecimalsName;
                    }

                    if (given != null)
                    {
                        component.AddError(
                            $"The input {given} needs the input {RowsName}.");
                        return false;
                    }
                    return true;
                }

                var rows = new List<Row>();
                foreach (var rowName in rowNames)
                {
                    var trimmed = (rowName ?? "").Trim();
                    var row = Rows.FirstOrDefault(
                        candidate => string.Equals(
                            candidate.Name,
                            trimmed,
                            StringComparison.OrdinalIgnoreCase));
                    if (row == null)
                    {
                        component.AddError(
                            $"Unknown {RowWord} in the input {RowsName}: \"{rowName}\". It has to be one of {RowNames}.");
                        return false;
                    }
                    if (rows.Contains(row))
                    {
                        component.AddError(
                            $"The {RowWord} {row.Name} is given twice in the input {RowsName}.");
                        return false;
                    }
                    rows.Add(row);
                }

                if (!CheckColumnSize(component, units.Count, rows.Count, UnitsName) ||
                    !CheckColumnSize(component, decimals.Count, rows.Count, DecimalsName) ||
                    !CheckColumnSize(component, accuracies.Count, rows.Count, AccuraciesName) ||
                    !CheckColumnSize(component, hideZeroDecimals.Count, rows.Count, HideZeroDecimalsName))
                {
                    return false;
                }

                if (accuracies.Count == 1 &&
                    !string.IsNullOrWhiteSpace(accuracies[0]) &&
                    !rows.Any(row => row.HasAccuracy))
                {
                    component.AddError(
                        $"None of the given {RowWord}s has an extra accuracy, so the input {AccuraciesName} cannot be applied.");
                    return false;
                }

                var page = Child(parameters, PageKey);
                for (var i = 0; i < rows.Count; i++)
                {
                    var fields = Child(page, rows[i].Key);

                    if (units.Count > 0)
                    {
                        var unit = ValueAt(units, i);
                        if (!string.IsNullOrWhiteSpace(unit))
                        {
                            fields["unit"] = unit.Trim();
                        }
                    }

                    if (decimals.Count > 0)
                    {
                        fields["decimals"] = ValueAt(decimals, i);
                    }

                    if (accuracies.Count > 0)
                    {
                        var accuracy = ValueAt(accuracies, i);
                        if (!string.IsNullOrWhiteSpace(accuracy))
                        {
                            if (rows[i].HasAccuracy)
                            {
                                fields["accuracy"] = accuracy.Trim();
                            }
                            else if (accuracies.Count > 1)
                            {
                                component.AddError(
                                    $"The {RowWord} {rows[i].Name} has no extra accuracy; leave its item of the input {AccuraciesName} empty.");
                                return false;
                            }
                            // A single accuracy for all rows is applied to the
                            // rows that have one.
                        }
                    }

                    if (hideZeroDecimals.Count > 0)
                    {
                        fields["hideZeroDecimals"] = ValueAt(hideZeroDecimals, i);
                    }
                }

                return true;
            }

            // A column is empty, one value for every row, or one value per row.
            private bool CheckColumnSize(
                Component component,
                int count,
                int rowCount,
                string columnName)
            {
                if (count == 0 ||
                    count == 1 ||
                    count == rowCount)
                {
                    return true;
                }

                component.AddError(
                    $"The size of the input {columnName} must be 1 or equal to the size of the input {RowsName}.");
                return false;
            }

            private static T ValueAt<T>(
                List<T> column,
                int index)
            {
                return column[column.Count == 1 ? 0 : index];
            }
        }

        // The Working Units page: the integer angle accuracy and the round
        // inch of the length have no column, they go through the JSON.
        public static readonly Table WorkingUnits = new Table(
            "workingUnits",
            "Working Units",
            "quantity",
            new[]
            {
                new Row("Length", "length", false),
                new Row("Area", "area", false),
                new Row("Volume", "volume", false),
                new Row("Angle", "angle", false)
            },
            "WorkingUnitQuantities",
            "WorkingUnits",
            "WorkingUnitDecimals",
            null,
            null);

        public static readonly Table CalculationUnits = new Table(
            "calculationUnits",
            "Calculation Units",
            "quantity",
            new[]
            {
                new Row("Length", "length", true),
                new Row("Area", "area", true),
                new Row("Volume", "volume", true),
                new Row("Angle", "angle", false)
            },
            "CalculationUnitQuantities",
            "CalculationUnits",
            "CalculationUnitDecimals",
            "CalculationUnitAccuracies",
            null);

        // The Dimensions page: the linear-type rows take length units, Angle
        // an angle unit and Area an area unit. The dimension standard, the
        // round inch and the zero whole/inch flags go through the JSON.
        public static readonly Table Dimensions = new Table(
            "dimensions",
            "Dimensions",
            "dimension type",
            new[]
            {
                new Row("Linear", "linear", true),
                new Row("Angle", "angle", false),
                new Row("Radial", "radial", true),
                new Row("Level", "level", true),
                new Row("Elevation", "elevation", true),
                new Row("DoorWindow", "doorWindow", true),
                new Row("Parapet", "parapet", true),
                new Row("Area", "area", false)
            },
            "DimensionTypes",
            "DimensionUnits",
            "DimensionDecimals",
            "DimensionAccuracies",
            "DimensionHideZeroDecimals");

        public static readonly Table[] Tables =
        {
            WorkingUnits,
            CalculationUnits,
            Dimensions
        };

        // The Zones page, as four plain inputs or outputs in this order: the
        // low ceiling area reduction, which is all the add-on exposes of the
        // page (the wall inset sizes are not in API_ZonePrefs).
        public const string ZoneActiveHeightLimitsName = "ZoneActiveHeightLimits";
        public const string ZoneHeightLimitsName = "ZoneHeightLimits";
        public const string ZoneReductionPercentsName = "ZoneReductionPercents";
        public const string ZoneReductionPercentAboveLimitsName = "ZoneReductionPercentAboveLimits";

        public const string ZoneActiveHeightLimitsDescription =
            "How many of the three ceiling height limits of the low ceiling area reduction are in use: 0 (no reduction) to 3.";

        public const string ZoneHeightLimitsDescription =
            "The three ceiling height limits of the low ceiling area reduction, in meters.";

        public const string ZoneReductionPercentsDescription =
            "The reduction of the zone area under each height limit (and above the previous one), in percent.";

        public const string ZoneReductionPercentAboveLimitsDescription =
            "The reduction of the zone area above the highest height limit, in percent.";

        // Hands the Zones page out, from the output at firstIndex on; empty
        // when the page is not in the response.
        public static void SetZoneOutputs(
            IGH_DataAccess da,
            JToken zones,
            int firstIndex)
        {
            object activeHeightLimits = null;
            var heightLimits = new List<object>();
            var reductionPercents = new List<object>();
            object reductionPercentAboveLimits = null;

            if (zones is JObject)
            {
                var reduction = zones["lowCeilingAreaReduction"];
                activeHeightLimits = JsonOutputHelp.Scalar(reduction, "activeHeightLimits");
                if (reduction?["heightLimits"] is JArray limits)
                {
                    foreach (var limit in limits)
                    {
                        heightLimits.Add(JsonOutputHelp.Scalar(limit, "height"));
                        reductionPercents.Add(JsonOutputHelp.Scalar(limit, "reductionPercent"));
                    }
                }
                reductionPercentAboveLimits = JsonOutputHelp.Scalar(reduction, "reductionPercentAboveHeightLimits");
            }

            da.SetData(firstIndex, activeHeightLimits);
            da.SetDataList(firstIndex + 1, heightLimits);
            da.SetDataList(firstIndex + 2, reductionPercents);
            da.SetData(firstIndex + 3, reductionPercentAboveLimits);
        }

        // Reads the Zones inputs from firstIndex on and puts the given fields
        // into the zones object of parameters. Returns false after reporting
        // an error on the component.
        public static bool TryApplyZoneInputs(
            Component component,
            IGH_DataAccess da,
            int firstIndex,
            JObject parameters)
        {
            var activeHeightLimits = 0;
            if (da.GetData(firstIndex, ref activeHeightLimits))
            {
                Child(Child(parameters, "zones"), "lowCeilingAreaReduction")["activeHeightLimits"] = activeHeightLimits;
            }

            // The given items change the height limits of the same index.
            var heightLimits = da.GetOptionals<double>(firstIndex + 1);
            var reductionPercents = da.GetOptionals<double>(firstIndex + 2);
            if (heightLimits.Count > 3 ||
                reductionPercents.Count > 3)
            {
                component.AddError(
                    $"The inputs {ZoneHeightLimitsName} and {ZoneReductionPercentsName} take at most three items, one per height limit.");
                return false;
            }
            if (heightLimits.Count > 0 ||
                reductionPercents.Count > 0)
            {
                var limits = new JArray();
                for (var i = 0; i < Math.Max(heightLimits.Count, reductionPercents.Count); i++)
                {
                    var limit = new JObject();
                    if (i < heightLimits.Count)
                    {
                        limit["height"] = heightLimits[i];
                    }
                    if (i < reductionPercents.Count)
                    {
                        limit["reductionPercent"] = reductionPercents[i];
                    }
                    limits.Add(limit);
                }
                Child(Child(parameters, "zones"), "lowCeilingAreaReduction")["heightLimits"] = limits;
            }

            var value = 0.0;
            if (da.GetData(firstIndex + 3, ref value))
            {
                Child(Child(parameters, "zones"), "lowCeilingAreaReduction")["reductionPercentAboveHeightLimits"] = value;
            }

            return true;
        }

        // The object at key of parent, created when it is missing (or is not
        // an object, which the command would reject anyway).
        private static JObject Child(
            JObject parent,
            string key)
        {
            if (parent[key] is JObject child)
            {
                return child;
            }

            child = new JObject();
            parent[key] = child;
            return child;
        }
    }
}
