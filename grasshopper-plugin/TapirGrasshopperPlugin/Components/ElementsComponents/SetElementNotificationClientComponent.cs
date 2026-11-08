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
                "deleted, reserved and released elements, and on request the selection, window, project, " +
                "attribute replacement, tool change, Navigator item, Teamwork lockable object set reservation, " +
                "tool default, attribute, classification, property definition, marquee and 3D settings events, " +
                "as JSON to " +
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

            // Appended after ElementTypesToObserve: saved definitions find the inputs by position.
            inManager.AddBooleanParameter(
                "NotifyOnToolChange",
                "NotifyOnToolChange",
                "Notify when the user switches to another tool in the Toolbox, as toolChanged with the element type of the new tool (None for the Arrow and the Marquee). Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnViewEvents",
                "NotifyOnViewEvents",
                "Notify when a Navigator item (a view, story, layout, publisher set item) is inserted, modified, deleted or opened, as viewEvent with the event, the navigator map and the item's guid. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnLockableReservationChanges",
                "NotifyOnLockableReservationChanges",
                "Notify when a Teamwork lockable object set (attributes of a type, favorites, model view options, project info, ...) is reserved or released, as lockableReservationChanged with the object set's guid and name and who reserved it. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnToolDefaultChanges",
                "NotifyOnToolDefaultChanges",
                "Notify when the default settings of a tool change, as toolDefaultsChanged with the element type whose defaults changed, so a tool component can refresh. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnAttributeChanges",
                "NotifyOnAttributeChanges",
                "Notify when attributes are created, modified or deleted, as attributesChanged with the event and the attribute ids. Archicad 26 and later. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnClassificationChanges",
                "NotifyOnClassificationChanges",
                "Notify when classification systems or items are created, modified or deleted, as classificationSystemsChanged and classificationItemsChanged. Archicad 26 and later. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnPropertyDefinitionChanges",
                "NotifyOnPropertyDefinitionChanges",
                "Notify when property groups or property definitions are created, modified or deleted, as propertyGroupsChanged and propertyDefinitionsChanged. Archicad 26 and later. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOnMarqueeChange",
                "NotifyOnMarqueeChange",
                "Notify when the marquee is drawn, changed or removed, as marqueeChanged with its type. Archicad 27 and later. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "NotifyOn3DSettingsChanges",
                "NotifyOn3DSettingsChanges",
                "Notify when the settings of the 3D window change, as view3DSettingsChanged. Archicad 30 and later. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(new[] { 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 });
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

            var notifyOnToolChange = false;
            if (da.GetData(10, ref notifyOnToolChange))
            {
                parameters["notifyOnToolChange"] = notifyOnToolChange;
            }

            var notifyOnViewEvents = false;
            if (da.GetData(11, ref notifyOnViewEvents))
            {
                parameters["notifyOnViewEvents"] = notifyOnViewEvents;
            }

            var notifyOnLockableReservationChanges = false;
            if (da.GetData(12, ref notifyOnLockableReservationChanges))
            {
                parameters["notifyOnLockableReservationChanges"] = notifyOnLockableReservationChanges;
            }

            var notifyOnToolDefaultChanges = false;
            if (da.GetData(13, ref notifyOnToolDefaultChanges))
            {
                parameters["notifyOnToolDefaultChanges"] = notifyOnToolDefaultChanges;
            }

            var notifyOnAttributeChanges = false;
            if (da.GetData(14, ref notifyOnAttributeChanges))
            {
                parameters["notifyOnAttributeChanges"] = notifyOnAttributeChanges;
            }

            var notifyOnClassificationChanges = false;
            if (da.GetData(15, ref notifyOnClassificationChanges))
            {
                parameters["notifyOnClassificationChanges"] = notifyOnClassificationChanges;
            }

            var notifyOnPropertyDefinitionChanges = false;
            if (da.GetData(16, ref notifyOnPropertyDefinitionChanges))
            {
                parameters["notifyOnPropertyDefinitionChanges"] = notifyOnPropertyDefinitionChanges;
            }

            var notifyOnMarqueeChange = false;
            if (da.GetData(17, ref notifyOnMarqueeChange))
            {
                parameters["notifyOnMarqueeChange"] = notifyOnMarqueeChange;
            }

            var notifyOn3DSettingsChanges = false;
            if (da.GetData(18, ref notifyOn3DSettingsChanges))
            {
                parameters["notifyOn3DSettingsChanges"] = notifyOn3DSettingsChanges;
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
