using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetElementNotificationClientComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetElementNotificationClient";

        public SetElementNotificationClientComponent()
            : base(
                "SetElementNotificationClient",
                "Sets up a new notification client to receive element events: Archicad posts the new, changed, " +
                "deleted, reserved and released elements as JSON to http://<Host>:<Port>/element_notification. " +
                "Setting up the same client again changes which events it gets. The client (an HTTP server) is " +
                "not part of this component; remove it with RemoveElementNotificationClient.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Host",
                "The host address of the notification client. Defaults to localhost. Optional.");

            inManager.AddIntegerParameter(
                "Port",
                "Port",
                "The port number of the notification client."
                    .WithTypeName("integerItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnNewElement",
                "NotifyOnNewElement",
                "Notify on the creation of a new element. Defaults to true. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnModificationOfAnElement",
                "NotifyOnModificationOfAnElement",
                "Notify on the modification or deletion of an element. Defaults to true. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnReservationChanges",
                "NotifyOnReservationChanges",
                "Notify on the reservation changes of an element. Defaults to true. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(new[] { 0, 2, 3, 4 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var port = 0;
            if (!da.GetData(1, ref port))
            {
                return;
            }

            var parameters = new JObject { ["port"] = port };

            string host = null;
            if (da.GetData(0, ref host) &&
                !string.IsNullOrEmpty(host))
            {
                parameters["host"] = host;
            }

            var notifyOnNewElement = true;
            if (da.GetData(2, ref notifyOnNewElement))
            {
                parameters["notifyOnNewElement"] = notifyOnNewElement;
            }

            var notifyOnModificationOfAnElement = true;
            if (da.GetData(3, ref notifyOnModificationOfAnElement))
            {
                parameters["notifyOnModificationOfAnElement"] = notifyOnModificationOfAnElement;
            }

            var notifyOnReservationChanges = true;
            if (da.GetData(4, ref notifyOnReservationChanges))
            {
                parameters["notifyOnReservationChanges"] = notifyOnReservationChanges;
            }

            if (TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                this.CheckExecutionResult(response);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetElementNotificationClient;

        public override Guid ComponentGuid =>
            new Guid("7ed6e613-ee00-4c2a-ae73-f602f96a7252");
    }
}
