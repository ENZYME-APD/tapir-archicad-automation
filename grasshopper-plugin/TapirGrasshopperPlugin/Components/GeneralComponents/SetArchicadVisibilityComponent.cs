using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class SetArchicadVisibilityComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetArchicadVisibility";

        public SetArchicadVisibilityComponent()
            : base(
                "SetArchicadVisibility",
                "Hides or shows the Archicad application, e.g. to keep it out of the way while a batch runs. " +
                "A definition that hides Archicad must show it again, also when a later step fails. " +
                "Supported on Windows only; on macOS it returns an error.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddBooleanParameter(
                "IsVisible",
                "IsVisible",
                "True shows the Archicad application, false hides it."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out bool isVisible))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["isVisible"] = isVisible },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            if ((bool?)response?["success"] != true)
            {
                this.AddError(
                    response?["error"]?["message"]?.ToString() ??
                    "Failed to change the visibility of Archicad.");
            }
        }

        // Shares the icon of ArchicadLocation until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ArchicadLocation;

        public override Guid ComponentGuid =>
            new Guid("1a88efd8-c14a-4dcf-bc47-091f3ba751ea");
    }
}
