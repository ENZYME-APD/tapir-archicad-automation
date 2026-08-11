using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetSuspendGroupsModeComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetSuspendGroupsMode";

        public SetSuspendGroupsModeComponent()
            : base(
                "SetSuspendGroupsMode",
                "Turn the Suspend Groups mode on or off. " +
                "Suspend groups to perform operations on elements that are part of a group; " +
                "remember to restore the previous state afterwards.",
                GroupNames.ElementGrouping)
        {
        }

        protected override void AddInputs()
        {
            InBoolean(
                "Suspend",
                "Turn the Suspend Groups mode on or off.",
                true);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out bool suspend))
            {
                return;
            }

            var parameters = new JObject { ["suspendGroups"] = suspend };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            // The command reports a failed switch in its executionResult
            // instead of failing the call itself.
            var result = response["executionResult"];
            if (result != null &&
                (bool?)result["success"] != true)
            {
                this.AddError(
                    result["error"]?["message"]?.ToString() ??
                    "Failed to change the Suspend Groups mode.");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetSuspendGroupsMode;

        public override Guid ComponentGuid =>
            new Guid("43e968db-e18d-46cf-b5a1-9240399560b1");
    }
}
