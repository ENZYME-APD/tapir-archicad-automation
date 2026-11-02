using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class SetDialogAutoCloseComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetDialogAutoClose";

        public SetDialogAutoCloseComponent()
            : base(
                "SetDialogAutoClose",
                "Turns the dialog auto close on or off. While it is on, Archicad closes the dialogs it would show with " +
                "their default button instead of waiting for the user, so automation is not blocked by them. To have it " +
                "on already while Archicad opens a project, set the TAPIR_DIALOG_AUTOCLOSE environment variable to true " +
                "for the Archicad process before starting it.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddBooleanParameter(
                "Enabled",
                "Enabled",
                "True turns the dialog auto close on, false turns it off."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out bool enabled))
            {
                return;
            }

            var parameters = new JObject
            {
                ["enabled"] = enabled
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            if ((bool?)response?["success"] != true)
            {
                this.AddError(
                    response?["error"]?["message"]?.ToString() ??
                    "Failed to change the dialog auto close.");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetDialogAutoClose;

        public override Guid ComponentGuid =>
            new Guid("6ea968f9-e117-4ce8-9301-0c9a8f2a1591");
    }
}
