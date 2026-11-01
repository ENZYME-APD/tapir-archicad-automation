using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.TeamworkComponents
{
    public abstract class AbsTeamworkComponent : AbsTeamworkBaseComponent
    {
        protected AbsTeamworkComponent(
            string name,
            string description)
            : base(
                name,
                description)
        {
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            da.SetData(
                0,
                response.Message());
        }
    }

    public class TeamworkComponents : AbsTeamworkComponent
    {
        public override string CommandName => "TeamworkReceive";

        public TeamworkComponents()
            : base(
                "TeamworkReceive",
                "Performs a receive operation on the currently opened Teamwork project.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.TeamworkReceive;

        public override Guid ComponentGuid =>
            new Guid("8ee23aed-ee26-4966-ba00-6d718e844dbd");
    }

    public class TeamworkSendComponent : AbsTeamworkComponent
    {
        public override string CommandName => "TeamworkSend";

        public TeamworkSendComponent()
            : base(
                "TeamworkSend",
                "Performs a send operation on the currently opened Teamwork project.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.TeamworkSend;

        public override Guid ComponentGuid =>
            new Guid("2684e26e-34f1-4315-89f8-136d46e54c52");
    }

    public class GetTeamworkProjectInfoComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetTeamworkProjectInfo";

        public GetTeamworkProjectInfoComponent()
            : base(
                "TeamworkProjectInfo",
                "Tells whether the opened project is a Teamwork project and whether it is online, " +
                "with its BIMcloud server, project name and the current user. " +
                "In a solo project only IsTeamwork and IsOnline are set, both to false.",
                GroupNames.Teamwork)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "IsTeamwork",
                "True if the opened project is a Teamwork (BIMcloud) project.");

            OutBoolean(
                "IsOnline",
                "True if the Teamwork project is connected to its BIMcloud server; " +
                "false in a solo project or when the Teamwork project works offline.");

            OutInteger(
                "UserId",
                "Teamwork user id of the current user, as the Teamwork components report users " +
                "and GrantElements expects them.");

            OutText(
                "UserName",
                "Name of the current Teamwork user.");

            OutText(
                "ServerUrl",
                "Address of the BIMcloud server of the Teamwork project.");

            OutText(
                "ProjectName",
                "Name of the Teamwork project on the server.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            // The Teamwork fields are left out of the response of a solo project,
            // so their outputs are null there.
            da.SetData(
                0,
                response.Value<bool?>("isTeamwork") ?? false);

            da.SetData(
                1,
                response.Value<bool?>("isOnline") ?? false);

            da.SetData(
                2,
                response.Value<int?>("userId"));

            da.SetData(
                3,
                response.Value<string>("userName"));

            da.SetData(
                4,
                response.Value<string>("serverUrl"));

            da.SetData(
                5,
                response.Value<string>("projectName"));
        }

        // Shares the icon of ProjectDetails until it gets its own, drawn in
        // IconSource/TeamworkProjectInfo.svg.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ProjectDetails;

        public override Guid ComponentGuid =>
            new Guid("6f2b9c41-7d3e-4a85-9b1c-2e8f5a7d3c91");
    }
}