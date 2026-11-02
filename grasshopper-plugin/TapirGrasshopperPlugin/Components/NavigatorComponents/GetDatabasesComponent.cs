using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetDatabasesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDatabases";

        public GetDatabasesComponent()
            : base(
                "GetDatabases",
                "Lists every database of the given types (sections, elevations, interior elevations, details, " +
                "worksheets, 3D documents, layouts, master layouts) with its database ID, name and reference ID, " +
                "without walking the navigator trees.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Types",
                "The types of the databases to list: Section, Elevation, InteriorElevation, Detail, Worksheet, " +
                "DocumentFrom3D, Layout or MasterLayout. Optional, by default the databases of every type are listed.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Types",
                "The type of each database.");

            OutGenerics(
                "DatabaseGuids",
                "The identifier of each database.");

            OutTexts(
                "Names",
                "The name of each database (null when the database info cannot be read).");

            OutTexts(
                "ReferenceIds",
                "The reference ID of each database (null when the database info cannot be read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGetList(
                    0,
                    out List<string> types) &&
                types.Count > 0)
            {
                parameters["types"] = new JArray(types);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var databaseTypes = new List<string>();
            var databaseIds = new List<DatabaseGuidWrapper>();
            var names = new List<string>();
            var referenceIds = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "databases"))
            {
                databaseTypes.Add(item["type"]?.ToString());
                databaseIds.Add(
                    new DatabaseGuidWrapper
                    {
                        DatabaseId = item["databaseId"]?.ToObject<DatabaseGuidObject>()
                    });
                names.Add(item["name"]?.ToString());
                referenceIds.Add(item["referenceId"]?.ToString());
            }

            da.SetDataList(
                0,
                databaseTypes);

            da.SetDataList(
                1,
                databaseIds);

            da.SetDataList(
                2,
                names);

            da.SetDataList(
                3,
                referenceIds);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDatabases;

        public override Guid ComponentGuid =>
            new Guid("23e3d427-7bc4-40b9-970e-5495d9249172");
    }
}
