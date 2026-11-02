using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class AddMEPPreferenceTableRowsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "AddMEPPreferenceTableRows";

        public AddMEPPreferenceTableRowsComponent()
            : base(
                "AddMEPPreferenceTableRows",
                "Adds rows with the given keys and optionally values to a branch, elbow or transition preference table " +
                "and returns their index, in one undo step. Branch tables from Archicad 28, elbow and transition tables " +
                "from Archicad 29; segment tables have a row for each reference id. On Archicad 27 a row is added to the " +
                "list of the given shape of any preference table, and the index of each new row after the call is " +
                "returned. Available from Archicad 27.",
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
                MEPPreferenceTableRowHelp.KeysDescription +
                " AddMEPPreferenceTableRows takes only the shape on Archicad 27.");

            InTexts(
                "Values",
                MEPPreferenceTableRowHelp.ValuesDescription +
                " (input only 1 to use the same values for all rows). Optional.");

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "Indices",
                "Index of each new row in the table. Archicad 27: in the list of its shape, after the call.");

            OutErrorMessages(
                "Error message of each row (empty when it was added).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!MEPComponentHelp.TryGetGuidText(this, da, 0, out var tableGuid) ||
                !MEPPreferenceTableRowHelp.TryGetKeys(this, da, 1, out var keys) ||
                !MEPPreferenceTableRowHelp.TryGetValues(this, da, 2, out var values) ||
                keys.Count == 0 ||
                !MEPPreferenceTableRowHelp.CheckCount(this, values.Count, keys.Count, "Values"))
            {
                return;
            }

            var rows = new JArray();
            for (var i = 0; i < keys.Count; i++)
            {
                var row = new JObject { ["key"] = keys[i] };
                if (values.Count > 0)
                {
                    row["values"] = values[values.Count == 1 ? 0 : i].DeepClone();
                }
                rows.Add(row);
            }

            var parameters = new JObject
            {
                ["tableId"] = MEPPreferenceTableRowHelp.TableId(tableGuid),
                ["rows"] = rows
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            MEPPreferenceTableRowHelp.SetRowResults(da, response, 0, 1);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AddMEPPreferenceTableRows;

        public override Guid ComponentGuid =>
            new Guid("1bcf12c0-c8f3-4124-a7fd-125670cf81d5");
    }
}
