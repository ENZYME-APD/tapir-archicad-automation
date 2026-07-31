using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class ShowAlertComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ShowAlert";

        public ShowAlertComponent()
            : base(
                "ShowAlert",
                "Display an alert dialog with up to three buttons in Archicad and return the clicked button.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Title",
                "Title of the alert dialog.");

            InText(
                "Message",
                "Main message text of the alert dialog.");

            InText(
                "SubMessage",
                "Smaller sub-message text below the main message. Optional.");

            InText(
                "AlertType",
                "Type of the alert dialog: information, warning or error. Defaults to information. Optional.");

            InText(
                "Button1",
                "Label of the first (default) button. Defaults to OK. Optional.");

            InText(
                "Button2",
                "Label of the second button. Optional.");

            InText(
                "Button3",
                "Label of the third button. Optional.");

            SetOptionality(new[] { 2, 3, 4, 5, 6 });
        }

        protected override void AddOutputs()
        {
            OutInteger(
                "ClickedButton",
                "Index of the button the user clicked: 1, 2 or 3.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string title = null;
            if (!da.GetData(0, ref title))
            {
                return;
            }

            string message = null;
            if (!da.GetData(1, ref message))
            {
                return;
            }

            var parameters = new JObject
            {
                ["alertType"] = "information",
                ["title"] = title,
                ["message"] = message,
                ["button1"] = "OK"
            };

            string subMessage = null;
            if (da.GetData(2, ref subMessage) && !string.IsNullOrEmpty(subMessage))
            {
                parameters["subMessage"] = subMessage;
            }

            string alertType = null;
            if (da.GetData(3, ref alertType) && !string.IsNullOrEmpty(alertType))
            {
                parameters["alertType"] = alertType;
            }

            string button1 = null;
            if (da.GetData(4, ref button1) && !string.IsNullOrEmpty(button1))
            {
                parameters["button1"] = button1;
            }

            string button2 = null;
            if (da.GetData(5, ref button2) && !string.IsNullOrEmpty(button2))
            {
                parameters["button2"] = button2;
            }

            string button3 = null;
            if (da.GetData(6, ref button3) && !string.IsNullOrEmpty(button3))
            {
                parameters["button3"] = button3;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(0, (int?)response["clickedButton"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ShowAlert;

        public override Guid ComponentGuid =>
            new Guid("b9af88c9-0d29-4b8a-b9e3-273814c75d52");
    }
}
