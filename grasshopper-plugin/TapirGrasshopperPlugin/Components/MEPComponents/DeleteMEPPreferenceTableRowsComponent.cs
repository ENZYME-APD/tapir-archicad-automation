using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class DeleteMEPPreferenceTableRowsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteMEPPreferenceTableRows";

        public DeleteMEPPreferenceTableRowsComponent()
            : base(
                "DeleteMEPPreferenceTableRows",
                "Deletes rows of a segment, branch, elbow or (from Archicad 29) transition preference table by their key " +
                "(the reference ids and, for branch and elbow rows from Archicad 29, the angle), in one undo step. The rows " +
                "of segment tables, and of the elbow tables of Archicad 28, are emptied instead. On Archicad 27 a row is " +
                "found by its shape and index. Available from Archicad 27.",
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
            OutErrorMessages(
                "Error message of each row (empty when it was deleted).");
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

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteMEPPreferenceTableRows;

        public override Guid ComponentGuid =>
            new Guid("fc8bd2e8-7551-43de-ae4e-9a4436f793e8");
    }
}
