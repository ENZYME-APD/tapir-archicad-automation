using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPPreferenceTableRowsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPPreferenceTableRows";

        public GetMEPPreferenceTableRowsComponent()
            : base(
                "GetMEPPreferenceTableRows",
                "Gets rows of a segment, branch, elbow or (from Archicad 29) transition preference table by their key " +
                "(the reference ids and, for branch and elbow rows from Archicad 29, the angle): whether the row is valid " +
                "and its values (diameters, wall thickness, radius, length, angle, branch offset, connection length, " +
                "description), and whether the table exists. Rectangular duct and cable carrier segment tables only give " +
                "whether they exist, as their rows have no key. On Archicad 27, which has one kind of preference table, " +
                "a row is found by its shape and index and has a value and a description; the table's name and number " +
                "of rows per shape are returned too. Available from Archicad 27.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "TableGuid",
                MEPPreferenceTableRowHelp.TableGuidDescription);

            InTexts(
                "Keys",
                MEPPreferenceTableRowHelp.KeysDescription);
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Exists",
                "Whether the table exists.");

            OutText(
                "Name",
                "Name of the table (Archicad 27 only).");

            OutInteger(
                "CircularRowCount",
                "Number of the circular rows of the table (Archicad 27 only).");

            OutInteger(
                "RectangularRowCount",
                "Number of the rectangular rows of the table (Archicad 27 only).");

            OutTexts(
                "Keys",
                "The key of each row as JSON.");

            OutBooleans(
                "IsValid",
                "Whether each row is valid. Archicad 27: whether the list of the shape has a row with the index.");

            OutTexts(
                "Values",
                "The values of each row as JSON, lengths in meters (the fields of the Values input of SetMEPPreferenceTableRows).");

            OutErrorMessages(
                "Error message of each row (empty when it was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!MEPComponentHelp.TryGetGuidText(this, da, 0, out var tableGuid) ||
                !MEPPreferenceTableRowHelp.TryGetKeys(this, da, 1, out var keys) ||
                keys.Count == 0)
            {
                return;
            }

            var parameters = new JObject
            {
                ["tableId"] = MEPPreferenceTableRowHelp.TableId(tableGuid),
                ["keys"] = new JArray(keys)
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var rowKeys = new List<object>();
            var isValid = new List<object>();
            var values = new List<object>();
            var errors = new List<string>();
            foreach (var row in JsonOutputHelp.Items(response, "rows"))
            {
                if (JsonOutputHelp.IsError(row))
                {
                    rowKeys.Add(null);
                    isValid.Add(null);
                    values.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(row));
                    continue;
                }
                rowKeys.Add(row["key"]?.ToString(Formatting.None));
                isValid.Add(JsonOutputHelp.Scalar(row, "valid"));
                values.Add(row["values"]?.ToString(Formatting.None));
                errors.Add("");
            }

            da.SetData(0, (bool?)response["exists"] ?? false);
            da.SetData(1, response["name"]?.ToString());
            da.SetData(2, (int?)response["sizes"]?["Circular"]);
            da.SetData(3, (int?)response["sizes"]?["Rectangular"]);
            da.SetDataList(4, rowKeys);
            da.SetDataList(5, isValid);
            da.SetDataList(6, values);
            da.SetDataList(7, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPPreferenceTableRows;

        public override Guid ComponentGuid =>
            new Guid("6b331552-76b2-4f71-9ef8-c33e025c1e0f");
    }
}
