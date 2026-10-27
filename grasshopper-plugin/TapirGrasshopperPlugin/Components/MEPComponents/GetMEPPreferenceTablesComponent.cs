using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPPreferenceTablesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPPreferenceTables";

        public GetMEPPreferenceTablesComponent()
            : base(
                "GetMEPPreferenceTables",
                "Get the segment preference tables of the given MEP domain: the circular tables, and the rectangular duct or cable carrier tables. Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Domain",
                "The MEP domain of the segment preference tables: Piping, Ventilation or CableCarrier.");
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
                "TableNames",
                "Names of the preference tables.");

            OutBooleans(
                "TablesEditable",
                "Whether each preference table is editable.");

            OutNumbers(
                "WallThicknesses",
                "Wall thickness of each table row (one branch per table).");

            OutTexts(
                "SizeTableGuids",
                "Identifiers of the rectangular duct (Ventilation) or cable carrier (CableCarrier) preference tables.");

            OutTexts(
                "SizeTableNames",
                "Names of the rectangular duct or cable carrier preference tables.");

            OutBooleans(
                "SizeTablesEditable",
                "Whether each rectangular duct or cable carrier preference table is editable.");

            OutNumbers(
                "Sizes",
                "Size of each rectangular duct or cable carrier table row (one branch per table).");

            OutTextTree(
                "SizeDescriptions",
                "Description of each rectangular duct or cable carrier table row (one branch per table).");
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

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var tableGuids = new List<string>();
            var tableNames = new List<string>();
            var tablesEditable = new List<bool>();
            var wallThicknesses = new DataTree<double>();
            var referenceIds = new DataTree<int>();
            var diameters = new DataTree<double>();
            var descriptions = new DataTree<string>();

            var tables = response["tables"] as JArray ?? new JArray();
            for (var i = 0; i < tables.Count; i++)
            {
                var table = tables[i];
                tableGuids.Add(table?["guid"]?.ToString());
                tableNames.Add(table?["name"]?.ToString() ?? "");
                tablesEditable.Add((bool?)table?["isEditable"] ?? false);

                var path = new GH_Path(i);
                referenceIds.EnsurePath(path);
                diameters.EnsurePath(path);
                descriptions.EnsurePath(path);
                wallThicknesses.EnsurePath(path);

                if (table?["rows"] is JArray rows)
                {
                    foreach (var row in rows)
                    {
                        referenceIds.Add((int?)row["referenceId"] ?? 0, path);
                        diameters.Add((double?)row["diameter"] ?? 0.0, path);
                        descriptions.Add(row["description"]?.ToString() ?? "", path);
                        wallThicknesses.Add((double?)row["wallThickness"] ?? 0.0, path);
                    }
                }
            }

            var sizeTableGuids = new List<string>();
            var sizeTableNames = new List<string>();
            var sizeTablesEditable = new List<bool>();
            var sizes = new DataTree<double>();
            var sizeDescriptions = new DataTree<string>();

            var sizeTables = new List<JToken>();
            sizeTables.AddRange(response["rectangularTables"] as JArray ?? new JArray());
            sizeTables.AddRange(response["cableCarrierTables"] as JArray ?? new JArray());
            for (var i = 0; i < sizeTables.Count; i++)
            {
                var table = sizeTables[i];
                sizeTableGuids.Add(table?["guid"]?.ToString());
                sizeTableNames.Add(table?["name"]?.ToString() ?? "");
                sizeTablesEditable.Add((bool?)table?["isEditable"] ?? false);

                var path = new GH_Path(i);
                sizes.EnsurePath(path);
                sizeDescriptions.EnsurePath(path);

                if (table?["rows"] is JArray rows)
                {
                    foreach (var row in rows)
                    {
                        sizes.Add((double?)row["value"] ?? 0.0, path);
                        sizeDescriptions.Add(row["description"]?.ToString() ?? "", path);
                    }
                }
            }

            da.SetDataList(0, tableGuids);
            da.SetDataTree(1, referenceIds);
            da.SetDataTree(2, diameters);
            da.SetDataTree(3, descriptions);
            da.SetDataList(4, tableNames);
            da.SetDataList(5, tablesEditable);
            da.SetDataTree(6, wallThicknesses);
            da.SetDataList(7, sizeTableGuids);
            da.SetDataList(8, sizeTableNames);
            da.SetDataList(9, sizeTablesEditable);
            da.SetDataTree(10, sizes);
            da.SetDataTree(11, sizeDescriptions);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPPreferenceTables;

        public override Guid ComponentGuid =>
            new Guid("6f1545ac-6b6f-4149-8db9-c6a8bfadb471");
    }
}
