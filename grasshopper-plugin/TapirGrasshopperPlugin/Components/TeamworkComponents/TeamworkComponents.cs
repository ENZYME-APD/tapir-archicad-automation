using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

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
            if (!TryGetConvertedValues(
                    CommandName,
                    null,
                    SendToAddOn,
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

        public override Guid ComponentGuid =>
            new Guid("2684e26e-34f1-4315-89f8-136d46e54c52");
    }
}