using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.TeamworkComponents
{
    public class GetTeamworkStatusComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetTeamworkStatus";

        public GetTeamworkStatusComponent()
            : base(
                "GetTeamworkStatus",
                "Returns whether the opened project is a Teamwork project and is online, the members of the " +
                "shared project and who has reserved the Hotlink and XRef management. Works in a solo project " +
                "too, then IsShared and IsOnline are false.",
                GroupNames.Teamwork)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "IsShared",
                "True if the opened project is a Teamwork (BIMcloud) project.");

            OutBoolean(
                "IsOnline",
                "True if there is a live connection to the Teamwork server; always false for a solo project.");

            OutIntegers(
                "UserIds",
                "Teamwork user id of each member of the shared project (empty for a solo project).");

            OutTexts(
                "UserNames",
                "Login name of each member.");

            OutTexts(
                "FullNames",
                "Full name of each member.");

            OutBooleans(
                "IsConnected",
                "True for each member who has joined the project.");

            OutInteger(
                "HotlinkCacheManagementOwnerId",
                "Teamwork user id of the user who has reserved the Hotlink and XRef management " +
                "(null when nobody has).");

            OutText(
                "HotlinkCacheManagementOwnerName",
                "Name of the user who has reserved the Hotlink and XRef management (null when nobody has).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var userIds = new List<int?>();
            var userNames = new List<string>();
            var fullNames = new List<string>();
            var isConnected = new List<bool?>();

            foreach (var user in JsonOutputHelp.Items(
                         response,
                         "users"))
            {
                userIds.Add(user.Value<int?>("userId"));
                userNames.Add(user.Value<string>("userName"));
                fullNames.Add(user.Value<string>("fullName"));
                isConnected.Add(user.Value<bool?>("isConnected"));
            }

            var owner = response["hotlinkCacheManagementOwner"];

            da.SetData(0, response.Value<bool?>("isShared") ?? false);
            da.SetData(1, response.Value<bool?>("isOnline") ?? false);
            da.SetDataList(2, userIds);
            da.SetDataList(3, userNames);
            da.SetDataList(4, fullNames);
            da.SetDataList(5, isConnected);
            da.SetData(6, owner?.Value<int?>("userId"));
            da.SetData(7, owner?.Value<string>("userName"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetTeamworkStatus;

        public override Guid ComponentGuid =>
            new Guid("65ddb18c-d7a6-4d23-9397-81294bdce3f2");
    }
}
