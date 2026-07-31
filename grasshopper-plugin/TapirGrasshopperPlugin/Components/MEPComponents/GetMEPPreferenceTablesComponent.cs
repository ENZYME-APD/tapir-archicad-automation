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
                "Get the circular segment preference tables of the given MEP domain. Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Domain",
                "The MEP domain of the segment preference tables: Piping or Ventilation.");
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
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPPreferenceTables;

        public override Guid ComponentGuid =>
            new Guid("6f1545ac-6b6f-4149-8db9-c6a8bfadb471");
    }
}
