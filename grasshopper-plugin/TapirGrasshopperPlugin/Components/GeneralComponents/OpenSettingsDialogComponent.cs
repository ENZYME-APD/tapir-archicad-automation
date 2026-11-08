using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class OpenSettingsDialogComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "OpenSettingsDialog";

        public OpenSettingsDialogComponent()
            : base(
                "OpenSettingsDialog",
                "Opens Archicad's settings dialog of the selected elements or the current tool (Archicad 28 and " +
                "later), or the default settings dialog of the given element type, waits for the user and reports " +
                "whether the dialog was closed with OK or Cancel. Archicad waits for the user, so it only runs on " +
                "the Execute button.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ElementType",
                "Opens the default settings dialog of this tool (an element type, e.g. Wall). Optional: without it " +
                "Archicad 28 and later open the settings of the selected elements, or of the current tool when " +
                "nothing is selected; Archicad 25 to 27 open the default settings of the current tool.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Cancelled",
                "True when the user closed the dialog with Cancel, false when with OK.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            string elementType = null;
            if (da.GetData(0, ref elementType) &&
                !string.IsNullOrEmpty(elementType))
            {
                parameters["elementType"] = elementType;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                (bool?)response["cancelled"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.OpenSettingsDialog;

        public override Guid ComponentGuid =>
            new Guid("2b10a6b6-0ead-4e4e-9673-c036d69a76a9");
    }
}
