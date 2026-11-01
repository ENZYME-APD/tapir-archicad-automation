using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPPreferenceTablesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPPreferenceTables";

        public GetMEPPreferenceTablesComponent()
            : base(
                "GetMEPPreferenceTables",
                "Get the circular cross section preference tables (referenceId, diameter, wall thickness, description) and " +
                "the reference set of the Piping or Ventilation domain, optionally with the elbow, branch and transition " +
                "preference tables, plus the rectangular duct tables of the Ventilation domain or the segment tables of the " +
                "CableCarrier domain. The *JSON outputs give each table complete (name, whether it is editable, " +
                "segmentation lengths from Archicad 29, and rows with their index and validity), in the shape " +
                "ModifyMEPPreferenceTables takes in its Tables input. The angle set and the transition tables are " +
                "available from Archicad 29. Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Domain",
                "The MEP domain of the segment preference tables: Piping, Ventilation or CableCarrier.");

            InBoolean(
                "IncludeFittingTables",
                "Also return the elbow, branch and transition preference tables (ignored for the CableCarrier domain).",
                false);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "TableGuids",
                "Identifiers of the preference tables.");

            OutIntegerTree(
                "ReferenceIds",
                "Reference id of each table row (one branch per table).");

            outManager.AddNumberParameter(
                "Diameters",
                "Diameters",
                "Diameter of each table row (one branch per table).",
                GH_ParamAccess.tree);

            OutTextTree(
                "Descriptions",
                "Description of each table row (one branch per table).");

            OutTexts(
                "TablesJSON",
                "Each circular segment preference table as JSON, complete.");

            OutTexts(
                "RectangularTablesJSON",
                "Each rectangular segment preference table of the Ventilation domain as JSON.");

            OutTexts(
                "CableCarrierTablesJSON",
                "Each segment preference table of the CableCarrier domain as JSON.");

            OutText(
                "ReferenceSetJSON",
                "The reference set of the Piping or Ventilation domain as JSON: which reference ids are defined.");

            OutText(
                "AngleSetJSON",
                "The angle set of the Piping or Ventilation domain as JSON: which angles are defined (Archicad 29 or newer).");

            OutTexts(
                "ElbowTablesJSON",
                "Each elbow preference table as JSON (only with IncludeFittingTables).");

            OutTexts(
                "BranchTablesJSON",
                "Each branch preference table as JSON (only with IncludeFittingTables).");

            OutTexts(
                "TransitionTablesJSON",
                "Each transition preference table as JSON (only with IncludeFittingTables, Archicad 29 or newer).");
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
            if (da.GetOptional(1, false))
            {
                parameters["includeFittingTables"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var tableGuids = new List<string>();
            var referenceIds = new DataTree<int>();
            var diameters = new DataTree<double>();
            var descriptions = new DataTree<string>();

            var tables = response["tables"] as JArray ?? new JArray();
            for (var i = 0; i < tables.Count; i++)
            {
                var table = tables[i];
                tableGuids.Add(table?["guid"]?.ToString());

                var path = new GH_Path(i);
                referenceIds.EnsurePath(path);
                diameters.EnsurePath(path);
                descriptions.EnsurePath(path);

                if (table?["rows"] is JArray rows)
                {
                    foreach (var row in rows)
                    {
                        referenceIds.Add((int?)row["referenceId"] ?? 0, path);
                        diameters.Add((double?)row["diameter"] ?? 0.0, path);
                        descriptions.Add(row["description"]?.ToString() ?? "", path);
                    }
                }
            }

            da.SetDataList(0, tableGuids);
            da.SetDataTree(1, referenceIds);
            da.SetDataTree(2, diameters);
            da.SetDataTree(3, descriptions);
            da.SetDataList(4, JsonTexts(response["tables"]));
            da.SetDataList(5, JsonTexts(response["rectangularTables"]));
            da.SetDataList(6, JsonTexts(response["cableCarrierTables"]));
            da.SetData(7, response["referenceSet"]?.ToString(Formatting.None));
            da.SetData(8, response["angleSet"]?.ToString(Formatting.None));
            da.SetDataList(9, JsonTexts(response["elbowTables"]));
            da.SetDataList(10, JsonTexts(response["branchTables"]));
            da.SetDataList(11, JsonTexts(response["transitionTables"]));
        }

        private static List<string> JsonTexts(
            JToken array)
        {
            var texts = new List<string>();
            if (array is JArray items)
            {
                foreach (var item in items)
                {
                    texts.Add(item.ToString(Formatting.None));
                }
            }
            return texts;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPPreferenceTables;

        public override Guid ComponentGuid =>
            new Guid("6f1545ac-6b6f-4149-8db9-c6a8bfadb471");
    }
}
