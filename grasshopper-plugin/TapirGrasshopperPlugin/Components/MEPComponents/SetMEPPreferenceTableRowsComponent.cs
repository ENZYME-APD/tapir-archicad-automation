using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class SetMEPPreferenceTableRowsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetMEPPreferenceTableRows";

        public SetMEPPreferenceTableRowsComponent()
            : base(
                "SetMEPPreferenceTableRows",
                "Sets the given values of rows of a segment, branch, elbow or (from Archicad 29) transition preference " +
                "table, found by their key (the reference ids and, for branch and elbow rows from Archicad 29, the angle). " +
                "Only the given values are changed, in one undo step. On Archicad 27 a row is found by its shape and " +
                "index, its value and description are set, and its index after the call is returned; the keys give the " +
                "indices before the call. Available from Archicad 27.",
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

            InTexts(
                "Values",
                MEPPreferenceTableRowHelp.ValuesDescription +
                " Only the given values are set (input only 1 to set the same values in all rows).");
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "Indices",
                "Archicad 27 only: the index of each row after the call, which can differ from the index of its key.");

            OutErrorMessages(
                "Error message of each row (empty when it was set).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!MEPComponentHelp.TryGetGuidText(this, da, 0, out var tableGuid) ||
                !MEPPreferenceTableRowHelp.TryGetKeys(this, da, 1, out var keys) ||
                !MEPPreferenceTableRowHelp.TryGetValues(this, da, 2, out var values) ||
                keys.Count == 0 ||
                values.Count == 0 ||
                !MEPPreferenceTableRowHelp.CheckCount(this, values.Count, keys.Count, "Values"))
            {
                return;
            }

            var rows = new JArray();
            for (var i = 0; i < keys.Count; i++)
            {
                rows.Add(
                    new JObject
                    {
                        ["key"] = keys[i],
                        ["values"] = values[values.Count == 1 ? 0 : i].DeepClone()
                    });
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
            Properties.Resources.SetMEPPreferenceTableRows;

        public override Guid ComponentGuid =>
            new Guid("8da350df-b946-4e2f-aea3-fcea9a161e9c");
    }
}
