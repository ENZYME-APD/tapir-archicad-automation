using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.TeamworkComponents
{
    public class GetTeamworkOwnersOfDatabasesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetTeamworkOwnersOfDatabases";

        public GetTeamworkOwnersOfDatabasesComponent()
            : base(
                "GetTeamworkOwnersOfDatabases",
                "Returns the Teamwork owner (user id and name) of each given database, e.g. of a layout or " +
                "a view database. The user id is 0 when nobody owns the database.",
                GroupNames.Teamwork)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DatabaseIds",
                "The databases to get the owner of, for example the DatabaseIds output of GetContainingDatabases.");
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "UserIds",
                "Teamwork user id of the owner of each database; 0 when nobody owns it, null for failed items.");

            OutTexts(
                "UserNames",
                "Name of the owner of each database (null for failed items).");

            OutErrorMessages(
                "Error message of each database (empty when its owner was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var databases = DatabasesObject.Create(
                da,
                0);
            if (databases?.Databases == null)
            {
                this.AddError("Invalid database identifier in the DatabaseIds input.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["databases"] = JArray.FromObject(databases.Databases) },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var userIds = new List<object>();
            var userNames = new List<string>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "owners"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    userIds.Add(null);
                    userNames.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                userIds.Add(item.Value<int?>("userId"));
                userNames.Add(item["userName"]?.ToString());
                errorMessages.Add("");
            }

            da.SetDataList(
                0,
                userIds);

            da.SetDataList(
                1,
                userNames);

            da.SetDataList(
                2,
                errorMessages);
        }

        // Shares the icon of GetTeamworkStatus until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetTeamworkStatus;

        public override Guid ComponentGuid =>
            new Guid("95e99860-a09b-4f72-a523-7f290b91f048");
    }
}
