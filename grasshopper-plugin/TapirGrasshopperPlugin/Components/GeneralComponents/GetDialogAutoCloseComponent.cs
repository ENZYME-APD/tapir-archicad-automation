using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetDialogAutoCloseComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDialogAutoClose";

        public GetDialogAutoCloseComponent()
            : base(
                "GetDialogAutoClose",
                "Tells whether the dialog auto close is on, the same state SetDialogAutoClose writes, so a " +
                "definition can restore it afterwards. The state belongs to the running Archicad process: it " +
                "is off when Archicad starts (unless the TAPIR_DIALOG_AUTOCLOSE environment variable is true), " +
                "it stays as last set until Archicad quits, and it is shared by every add-on and script in " +
                "that Archicad.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Enabled",
                "True while Archicad closes the dialogs it would show with their default button instead " +
                "of waiting for the user.");
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
                response.Value<bool?>("enabled"));
        }

        // Shares the icon of SetDialogAutoClose until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetDialogAutoClose;

        public override Guid ComponentGuid =>
            new Guid("9a2798ee-efdf-48ec-a8f2-9123871bd42a");
    }
}
