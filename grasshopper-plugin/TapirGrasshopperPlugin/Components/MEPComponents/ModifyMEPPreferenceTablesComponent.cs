using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class ModifyMEPPreferenceTablesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyMEPPreferenceTables";

        public ModifyMEPPreferenceTablesComponent()
            : base(
                "ModifyMEPPreferenceTables",
                "Modifies the MEP preference tables of a domain, in the shape GetMEPPreferenceTables returns them: adds " +
                "tables, changes their names, segmentation lengths (from Archicad 29) and row values and descriptions, " +
                "deletes or empties rows, adds rows to the rectangular duct and cable carrier tables, deletes tables, and " +
                "deletes reference ids from the reference set and angles from the angle set (from Archicad 29). Each " +
                "table is changed in one undoable step. Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Domain",
                "The MEP domain of the preference tables: Piping, Ventilation or CableCarrier.");

            InTexts(
                "TablesToAddTypes",
                "Kind of each new table to add before the other changes: Segment, Elbow, Branch or Transition " +
                "(CableCarrier has only segment tables, transition tables from Archicad 29; " +
                "input only 1 to use the same kind for all). Optional.");

            InTexts(
                "TablesToAddNames",
                "Name of each new table to add. Optional.");

            InTexts(
                "Tables",
                "The tables to change, each as a JSON object in the shape the *TablesJSON outputs of " +
                "GetMEPPreferenceTables give them: guid, and the changes, e.g. name, useIndividualSegmentLengths, " +
                "minimumSegmentationLength, maximumSegmentationLength (from Archicad 29), rows " +
                "(each with its index and the values to change, or delete: true) and newRows " +
                "(rectangular duct and cable carrier tables). Only the given values and rows are changed. Optional.");

            InTexts(
                "TablesToDelete",
                "Guids of the tables of the domain to delete, after the other table changes. Optional.");

            InIntegers(
                "ReferenceIdsToDelete",
                "Reference ids to delete from the reference set of the Piping or Ventilation domain. Optional.");

            InNumbers(
                "AnglesToDelete",
                "Angles to delete from the angle set of the Piping or Ventilation domain, as GetMEPPreferenceTables " +
                "returns them (Archicad 29 or newer). Optional.");

            SetOptionality(new[] { 1, 2, 3, 4, 5, 6 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "AddedTableGuids",
                "Guid of each added table (empty when it failed).");

            OutTexts(
                "AddedTableErrorMessages",
                "Error message of each table to add (empty when it was added).");

            OutTexts(
                "TableErrorMessages",
                "Error message of each changed table (empty when it was changed).");

            OutTexts(
                "DeletedTableErrorMessages",
                "Error message of each table to delete (empty when it was deleted).");

            OutText(
                "ReferenceSetErrorMessage",
                "Error message of the deletion from the reference set (empty when it succeeded or was not asked).");

            OutText(
                "AngleSetErrorMessage",
                "Error message of the deletion from the angle set (empty when it succeeded or was not asked).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string domain = null;
            if (!da.GetData(0, ref domain))
            {
                return;
            }

            var parameters = new JObject { ["domain"] = domain };

            da.TryGetList(2, out List<string> addNames);
            addNames = addNames ?? new List<string>();
            var addFields = new ElectricalItemFields(this, 2, addNames.Count);
            if (!addFields.AddTexts(da, 1, "type"))
            {
                return;
            }
            if (addNames.Count > 0)
            {
                var tablesToAdd = new JArray();
                for (var i = 0; i < addNames.Count; i++)
                {
                    var table = new JObject();
                    addFields.ApplyTo(table, i);
                    if (table.Count == 0)
                    {
                        this.AddError("Give the kind of the tables to add in the TablesToAddTypes input.");
                        return;
                    }
                    table["name"] = addNames[i] ?? "";
                    tablesToAdd.Add(table);
                }
                parameters["tablesToAdd"] = tablesToAdd;
            }

            da.TryGetList(3, out List<string> tableTexts);
            if (tableTexts != null && tableTexts.Count > 0)
            {
                var tables = new JArray();
                foreach (var text in tableTexts)
                {
                    JObject table;
                    try
                    {
                        table = JObject.Parse(text ?? "");
                    }
                    catch (JsonReaderException ex)
                    {
                        this.AddError($"Invalid JSON object in the Tables input: {ex.Message}");
                        return;
                    }
                    if (table["guid"] == null)
                    {
                        this.AddError("Each table of the Tables input needs its guid.");
                        return;
                    }
                    tables.Add(table);
                }
                parameters["tables"] = tables;
            }

            if (!MEPComponentHelp.TryGetGuidTexts(this, da, 4, out var tablesToDelete))
            {
                return;
            }
            if (tablesToDelete.Count > 0)
            {
                parameters["tablesToDelete"] = new JArray(tablesToDelete);
            }

            da.TryGetList(5, out List<int> referenceIds);
            if (referenceIds != null && referenceIds.Count > 0)
            {
                parameters["referenceIdsToDelete"] = new JArray(referenceIds);
            }

            da.TryGetList(6, out List<double> angles);
            if (angles != null && angles.Count > 0)
            {
                parameters["anglesToDelete"] = new JArray(angles);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var addedGuids = new List<object>();
            var addedErrors = new List<string>();
            foreach (var added in JsonOutputHelp.Items(response, "addedTables"))
            {
                if (added?["guid"] != null)
                {
                    addedGuids.Add(added["guid"].ToString());
                    addedErrors.Add("");
                    continue;
                }
                addedGuids.Add(null);
                addedErrors.Add(added?["error"]?["message"]?.ToString() ?? "Unknown error.");
            }

            da.SetDataList(0, addedGuids);
            da.SetDataList(1, addedErrors);
            da.SetDataList(2, ExecutionResultsResponse.ErrorMessages(response, "tables"));
            da.SetDataList(3, ExecutionResultsResponse.ErrorMessages(response, "deletedTables"));
            da.SetData(4, MEPComponentHelp.ExecutionResultMessage(response["referenceSet"]) ?? "");
            da.SetData(5, MEPComponentHelp.ExecutionResultMessage(response["angleSet"]) ?? "");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMEPPreferenceTables;

        public override Guid ComponentGuid =>
            new Guid("e0014ebe-b24f-4e2c-9501-3cc78da9d5da");
    }
}
