using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetAutoGroupModeComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAutoGroupMode";

        public GetAutoGroupModeComponent()
            : base(
                "GetAutoGroupMode",
                "Get the current state of the Auto Group mode.",
                GroupNames.ElementGrouping)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "AutoGroup",
                "True if the Auto Group mode is currently on.");
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
                response["autoGroup"]?.Value<bool>());
        }

        // Shares the icon of GetSuspendGroupsMode.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetSuspendGroupsMode;

        public override Guid ComponentGuid =>
            new Guid("76586d79-b019-482e-8977-f3f888b5cd49");
    }
}
