using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetSuspendGroupsModeComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetSuspendGroupsMode";

        public GetSuspendGroupsModeComponent()
            : base(
                "GetSuspendGroupsMode",
                "Get the current state of the Suspend Groups mode.",
                GroupNames.Elements)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Suspend",
                "True if the Suspend Groups mode is currently on.");
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

            da.SetData(
                0,
                response["suspendGroups"]?.Value<bool>());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetSuspendGroupsMode;

        public override Guid ComponentGuid =>
            new Guid("f34c8c55-2f71-4dde-8c55-8e29b1061d6b");
    }
}
