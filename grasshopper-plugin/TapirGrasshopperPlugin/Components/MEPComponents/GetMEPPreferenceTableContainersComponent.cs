using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPPreferenceTableContainersComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPPreferenceTableContainers";

        public GetMEPPreferenceTableContainersComponent()
            : base(
                "GetMEPPreferenceTableContainers",
                "Lists the preference tables of the Piping, Ventilation and Cable Carrier preference table containers of " +
                "Archicad 27: their id, name, whether they exist and their number of rows per shape, and whether the " +
                "container exists. Tree outputs have one branch per container. Only for Archicad 27; from Archicad 28 " +
                "use GetMEPPreferenceTables.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Domains",
                "The domains whose preference tables to list: Piping, Ventilation or CableCarrier. " +
                "Optional, by default all three.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Domains",
                "Domain of each container.");

            OutBooleans(
                "ContainerExists",
                "Whether each container exists.");

            OutTextTree(
                "TableGuids",
                "Guid of each table, for the TableGuid of the preference table row components (one branch per container).");

            OutTextTree(
                "TableNames",
                "Name of each table (one branch per container).");

            OutBooleanTree(
                "TableExists",
                "Whether each table exists (one branch per container).");

            OutIntegerTree(
                "CircularRowCounts",
                "Number of the circular rows of each table (one branch per container).");

            OutIntegerTree(
                "RectangularRowCounts",
                "Number of the rectangular rows of each table (one branch per container).");

            OutErrorMessages(
                "Error message of each container (empty when it was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGetList(0, out List<string> domains);

            var parameters = new JObject();
            if (domains != null && domains.Count > 0)
            {
                parameters["domains"] = new JArray(domains);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var containerDomains = new List<object>();
            var containerExists = new List<object>();
            var tableGuids = new DataTree<object>();
            var tableNames = new DataTree<object>();
            var tableExists = new DataTree<object>();
            var circularCounts = new DataTree<object>();
            var rectangularCounts = new DataTree<object>();
            var errors = new List<string>();

            var containers = JsonOutputHelp.Items(response, "containers");
            for (var i = 0; i < containers.Count; i++)
            {
                var container = containers[i];
                var path = new GH_Path(i);
                tableGuids.EnsurePath(path);
                tableNames.EnsurePath(path);
                tableExists.EnsurePath(path);
                circularCounts.EnsurePath(path);
                rectangularCounts.EnsurePath(path);

                if (JsonOutputHelp.IsError(container))
                {
                    containerDomains.Add(null);
                    containerExists.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(container));
                    continue;
                }

                containerDomains.Add(JsonOutputHelp.Scalar(container, "domain"));
                containerExists.Add(JsonOutputHelp.Scalar(container, "exists"));
                errors.Add("");

                if (!(container["tables"] is JArray tables))
                {
                    continue;
                }

                foreach (var table in tables)
                {
                    tableGuids.Add(JsonOutputHelp.GuidOf(table["tableId"]), path);
                    tableNames.Add(JsonOutputHelp.Scalar(table, "name"), path);
                    tableExists.Add(JsonOutputHelp.Scalar(table, "exists"), path);
                    circularCounts.Add(JsonOutputHelp.Scalar(table["sizes"], "Circular"), path);
                    rectangularCounts.Add(JsonOutputHelp.Scalar(table["sizes"], "Rectangular"), path);
                }
            }

            da.SetDataList(0, containerDomains);
            da.SetDataList(1, containerExists);
            da.SetDataTree(2, tableGuids);
            da.SetDataTree(3, tableNames);
            da.SetDataTree(4, tableExists);
            da.SetDataTree(5, circularCounts);
            da.SetDataTree(6, rectangularCounts);
            da.SetDataList(7, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPPreferenceTableContainers;

        public override Guid ComponentGuid =>
            new Guid("832c9efe-ca3a-48b2-9d7f-bd4280925ccc");
    }
}
