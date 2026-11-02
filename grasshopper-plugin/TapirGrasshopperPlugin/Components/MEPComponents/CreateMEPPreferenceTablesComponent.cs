using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class CreateMEPPreferenceTablesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateMEPPreferenceTables";

        public CreateMEPPreferenceTablesComponent()
            : base(
                "CreateMEPPreferenceTables",
                "Adds empty preference tables with the given names to the preference table container of a domain of " +
                "Archicad 27 and returns their id, in one undo step. Only for Archicad 27.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Domain",
                "The MEP domain of the preference table container: Piping, Ventilation or CableCarrier.");

            InTexts(
                "Names",
                "Name of each new table.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "TableGuids",
                "Guid of each new table, for the TableGuid of the preference table row components (empty when it failed).");

            OutErrorMessages(
                "Error message of each table (empty when it was created).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string domain = null;
            if (!da.GetData(0, ref domain) ||
                !da.TryGetList(
                    1,
                    out List<string> names))
            {
                return;
            }

            var tables = new JArray();
            foreach (var name in names)
            {
                tables.Add(new JObject { ["name"] = name ?? "" });
            }

            var parameters = new JObject
            {
                ["domain"] = domain,
                ["tables"] = tables
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var tableGuids = new List<object>();
            var errors = new List<string>();
            foreach (var result in JsonOutputHelp.Items(response, "executionResults"))
            {
                if ((bool?)result?["success"] == true)
                {
                    tableGuids.Add(JsonOutputHelp.GuidOf(result["tableId"]));
                    errors.Add("");
                    continue;
                }
                tableGuids.Add(null);
                errors.Add(result?["error"]?["message"]?.ToString() ?? "Unknown error.");
            }

            da.SetDataList(0, tableGuids);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMEPPreferenceTables;

        public override Guid ComponentGuid =>
            new Guid("fc62d67d-28b7-473a-aeb9-6d035f637664");
    }
}
