using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetContainingDatabasesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetContainingDatabases";

        public GetContainingDatabasesComponent()
            : base(
                "GetContainingDatabases",
                "Returns the database (window type, database id and name) that contains each of the given elements, " +
                "from any database, not only the current one. Pass the WindowType and DatabaseId to ChangeWindow " +
                "to open it, for example to find an element placed on a layout.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to look up.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "DatabaseIds",
                "The database of each element (null for failed items).");

            OutTexts(
                "WindowTypes",
                "The window type of the database of each element.");

            OutTexts(
                "Names",
                "The name of the database of each element (empty for databases without a name, like the floor plan).");

            OutErrorMessages(
                "Error message of each element (empty when its database was found).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var databaseIds = new List<DatabaseGuidObject>();
            var windowTypes = new List<string>();
            var names = new List<string>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "databases"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    databaseIds.Add(null);
                    windowTypes.Add(null);
                    names.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                var guid = JsonOutputHelp.GuidOf(item["databaseId"]);
                databaseIds.Add(
                    guid == null
                        ? null
                        : new DatabaseGuidObject { Guid = guid });
                windowTypes.Add(item["windowType"]?.ToString());
                names.Add(item["name"]?.ToString());
                errorMessages.Add("");
            }

            da.SetDataList(
                0,
                databaseIds);

            da.SetDataList(
                1,
                windowTypes);

            da.SetDataList(
                2,
                names);

            da.SetDataList(
                3,
                errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetContainingDatabases;

        public override Guid ComponentGuid =>
            new Guid("18ac94ea-50ae-47c9-9b72-0b1ea2062c5b");
    }
}
