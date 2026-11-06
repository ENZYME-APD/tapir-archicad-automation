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
                "deleted, reserved and released elements, and on request the selection, window, project and " +
                "attribute replacement events, as JSON to " +
                "http://<Host>:<Port>/element_notification. " +
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

            inManager.AddBooleanParameter(
                "NotifyOnSelectionChange",
                "NotifyOnSelectionChange",
                "Notify when the selection changes, with the whole current selection as selectedElements. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnWindowChange",
                "NotifyOnWindowChange",
                "Notify when the current window or the active story changes, as windowChanged with the new window's type, databaseId and story index. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnProjectEvents",
                "NotifyOnProjectEvents",
                "Notify on project events (New, NewAndReset, Open, Close, Quit, Teamwork SendChanges and ReceiveChanges) as projectEvent. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnAttributeReplacement",
                "NotifyOnAttributeReplacement",
                "Notify when attributes are deleted and replaced by others (Delete and Replace, Attribute Manager), as replacedAttributes with the attribute type and the old and new index. Not available in Archicad 25. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddTextParameter(
                "ElementTypesToObserve",
                "ElementTypesToObserve",
                ("Only the modification or deletion of elements of these types (e.g. Wall, Column) is notified. " +
                 "Defaults to every element. Archicad is blocked while it attaches an observer to each element " +
                 "to observe, for minutes on projects with thousands of elements, so limit the types on large " +
                 "projects. Optional.")
                    .WithTypeName("textList"),
                GH_ParamAccess.list);

            SetOptionality(new[] { 0, 2, 3, 4, 5, 6, 7, 8, 9 });
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

            var notifyOnSelectionChange = false;
            if (da.GetData(5, ref notifyOnSelectionChange))
            {
                parameters["notifyOnSelectionChange"] = notifyOnSelectionChange;
            }

            var notifyOnWindowChange = false;
            if (da.GetData(6, ref notifyOnWindowChange))
            {
                parameters["notifyOnWindowChange"] = notifyOnWindowChange;
            }

            var notifyOnProjectEvents = false;
            if (da.GetData(7, ref notifyOnProjectEvents))
            {
                parameters["notifyOnProjectEvents"] = notifyOnProjectEvents;
            }

            var notifyOnAttributeReplacement = false;
            if (da.GetData(8, ref notifyOnAttributeReplacement))
            {
                parameters["notifyOnAttributeReplacement"] = notifyOnAttributeReplacement;
            }

            var elementTypesToObserve = new System.Collections.Generic.List<string>();
            if (da.GetDataList(9, elementTypesToObserve) &&
                elementTypesToObserve.Count > 0)
            {
                parameters["elementTypesToObserve"] = new JArray(elementTypesToObserve);
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
