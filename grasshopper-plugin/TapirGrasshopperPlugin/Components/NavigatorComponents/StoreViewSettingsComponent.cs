using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class StoreViewSettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "StoreViewSettings";

        public StoreViewSettingsComponent()
            : base(
                "StoreViewSettings",
                "Store the current view settings (Action Store) and put them back later (Action Restore), for " +
                "example before and after a definition that changes the view. The two must be used in pairs and " +
                "cannot be stacked. Restoring may rebuild the view.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InTextWithDefault(
                "Action",
                "Store to save the current view settings, Restore to put the stored ones back.",
                "Store");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGet(
                0,
                out string action);
            if (action != "Store" && action != "Restore")
            {
                this.AddError("The input Action must be Store or Restore.");
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new JObject { ["action"] = action },
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }
        }

        // Shares the icon of the 3D projection setter until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Set3DProjectionSettings;

        public override Guid ComponentGuid =>
            new Guid("d5773fe7-415c-410c-a257-504e6ab22491");
    }
}
