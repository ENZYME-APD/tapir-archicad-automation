using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.TeamworkComponents
{
    // The reservation of the non-element things of a Teamwork project: the
    // lockable object sets (attributes, favorites, project info, project
    // preferences, ...) addressed by their Archicad API name, and the Hotlink
    // and XRef management. Reserving and releasing change the reservations on
    // the Teamwork server, so those components run only on the Execute button.
    internal static class LockableObjectSetHelp
    {
        public const string ObjectSetNameDescription =
            "Name of the lockable object set as the Archicad API documents it: BuildingMaterials, " +
            "Cities, Composites, Favorites, FillTypes, LayerSettingsDialog (layers and layer " +
            "combinations), LineTypes, MarkupStyles, MEPSystems, ModelViewOptions, OperationProfiles, " +
            "PenTables, Profiles, ProjectInfo, GeoLocation (project location), PreferencesDialog " +
            "(project preferences and dimension standards), GraphicOverrides (renovation override " +
            "styles), Surfaces or ZoneCategories.";

        // The users in the conflicts array of a response ({userId, userName}).
        public static List<string> UserNames(
            JObject response)
        {
            var userNames = new List<string>();
            if (response?["conflicts"] is JArray conflicts)
            {
                foreach (var conflict in conflicts)
                {
                    userNames.Add(conflict?["userName"]?.ToString() ?? "");
                }
            }
            return userNames;
        }

        public static string ExecutionResultMessage(
            JToken executionResult)
        {
            return executionResult is JObject result
                ? ExecutionResult.Deserialize(result).Message()
                : "No execution result.";
        }

        public static bool TryGetObjectSetName(
            Component component,
            IGH_DataAccess da,
            out string objectSetName)
        {
            if (!da.TryGet(
                    0,
                    out objectSetName))
            {
                return false;
            }

            if (string.IsNullOrWhiteSpace(objectSetName))
            {
                component.AddError("The input ObjectSetName is empty.");
                return false;
            }

            return true;
        }
    }

    public class ReserveLockableObjectSetComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReserveLockableObjectSet";

        public ReserveLockableObjectSetComponent()
            : base(
                "ReserveLockableObjectSet",
                "Reserves a non-element lockable object set (attributes, favorites, project info, " +
                "project preferences, etc.) in Teamwork mode, e.g. Composites, LayerSettingsDialog or ProjectInfo.",
                GroupNames.Teamwork)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ObjectSetName",
                LockableObjectSetHelp.ObjectSetNameDescription);
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult),
                ExecutionResult.Doc);

            OutTexts(
                "ConflictUserNames",
                "Names of the users whose reservation prevented reserving the object set.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!LockableObjectSetHelp.TryGetObjectSetName(
                    this,
                    da,
                    out var objectSetName))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["objectSetName"] = objectSetName },
                    ToAddOn,
                    out var response))
            {
                return;
            }

            da.SetData(
                0,
                LockableObjectSetHelp.ExecutionResultMessage(response["executionResult"]));

            da.SetDataList(
                1,
                LockableObjectSetHelp.UserNames(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ReserveLockableObjectSet;

        public override Guid ComponentGuid =>
            new Guid("917987dd-8d85-4358-86d7-6c9c1cbda3e4");
    }

    public class ReleaseLockableObjectSetComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReleaseLockableObjectSet";

        public ReleaseLockableObjectSetComponent()
            : base(
                "ReleaseLockableObjectSet",
                "Releases a non-element lockable object set reserved with ReserveLockableObjectSet in Teamwork mode.",
                GroupNames.Teamwork)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ObjectSetName",
                LockableObjectSetHelp.ObjectSetNameDescription);
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!LockableObjectSetHelp.TryGetObjectSetName(
                    this,
                    da,
                    out var objectSetName))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["objectSetName"] = objectSetName },
                    ToAddOn,
                    out var response))
            {
                return;
            }

            da.SetData(
                0,
                LockableObjectSetHelp.ExecutionResultMessage(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ReleaseLockableObjectSet;

        public override Guid ComponentGuid =>
            new Guid("5e4ef8ad-4183-41a4-a6bb-d62da93d70f9");
    }

    public class GetLockableObjectSetStatusComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLockableObjectSetStatus";

        public GetLockableObjectSetStatusComponent()
            : base(
                "LockableObjectSetStatus",
                "Returns the Teamwork reservation status of a non-element lockable object set and the users who hold it.",
                GroupNames.Teamwork)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ObjectSetName",
                LockableObjectSetHelp.ObjectSetNameDescription);
        }

        protected override void AddOutputs()
        {
            OutText(
                "Status",
                "Free: nobody has reserved it. Editable: the current user has reserved it. " +
                "Locked: another user has reserved it. NotAvailable: the Teamwork server is not available. " +
                "NotExist: there is no Teamwork connection, e.g. the project is not a Teamwork project.");

            OutTexts(
                "UserNames",
                "Names of the users who hold a reservation on the object set.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!LockableObjectSetHelp.TryGetObjectSetName(
                    this,
                    da,
                    out var objectSetName))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["objectSetName"] = objectSetName },
                    ToAddOn,
                    out var response))
            {
                return;
            }

            // An unknown object set name is answered with an error, not a status.
            if (JsonOutputHelp.IsError(response))
            {
                this.AddError(JsonOutputHelp.ErrorMessage(response));
                return;
            }

            da.SetData(
                0,
                response["status"]?.ToString());

            da.SetDataList(
                1,
                LockableObjectSetHelp.UserNames(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLockableObjectSetStatus;

        public override Guid ComponentGuid =>
            new Guid("a1fc1b61-dada-4fef-b58d-17fd3f299d09");
    }

    public class ReserveHotlinkCacheManagementComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReserveHotlinkCacheManagement";

        public ReserveHotlinkCacheManagementComponent()
            : base(
                "ReserveHotlinkCacheManagement",
                "Reserves the Hotlink and XRef management in Teamwork mode, which is needed before changing hotlink nodes.",
                GroupNames.Teamwork)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult),
                ExecutionResult.Doc);

            OutTexts(
                "ConflictUserNames",
                "Name of the user whose reservation prevented reserving the Hotlink and XRef management.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            da.SetData(
                0,
                LockableObjectSetHelp.ExecutionResultMessage(response["executionResult"]));

            da.SetDataList(
                1,
                LockableObjectSetHelp.UserNames(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ReserveHotlinkCacheManagement;

        public override Guid ComponentGuid =>
            new Guid("b4bd6ae8-f27f-49ee-b629-009e0e66d251");
    }

    public class ReleaseHotlinkCacheManagementComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReleaseHotlinkCacheManagement";

        public ReleaseHotlinkCacheManagementComponent()
            : base(
                "ReleaseHotlinkCacheManagement",
                "Releases the Hotlink and XRef management in Teamwork mode.",
                GroupNames.Teamwork)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            da.SetData(
                0,
                LockableObjectSetHelp.ExecutionResultMessage(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ReleaseHotlinkCacheManagement;

        public override Guid ComponentGuid =>
            new Guid("80782b97-2e35-4a34-bcd6-504c8cd1f430");
    }
}
