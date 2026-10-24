using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.TeamworkComponents
{
    public abstract class AbsTeamworkElementsComponent
        : AbsTeamworkBaseComponent
    {
        protected AbsTeamworkElementsComponent(
            string name,
            string description)
            : base(
                name,
                description)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids to get detail list for.");
        }
    }

    public class ReserveElementsComponent : AbsTeamworkElementsComponent
    {
        public override string CommandName => "ReserveElements";

        public ReserveElementsComponent()
            : base(
                "ReserveElements",
                "Reserves elements in Teamwork mode.")
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult),
                ExecutionResult.Doc);

            OutGenerics($"{nameof(Conflict)}{nameof(Conflict.ElementId)}s");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject input))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
                    ToAddOn,
                    ExecutionResultResponse.Deserialize,
                    out ExecutionResultResponse response))
            {
                return;
            }

            da.SetData(
                0,
                response.Message());

            if (response.HasConflicts)
            {
                da.SetData(
                    1,
                    response.Conflicts.Select(x => x.ElementId));
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ReserveElements;

        public override Guid ComponentGuid =>
            new Guid("3c0e9944-2875-4a68-8794-ec16fa3235f5");
    }

    public class ReleaseElementsComponent : AbsTeamworkElementsComponent
    {
        public override string CommandName => "ReleaseElements";

        public ReleaseElementsComponent()
            : base(
                "ReleaseElements",
                "Releases elements in Teamwork mode.")
        {
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject input))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            da.SetData(
                0,
                response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ReleaseElements;

        public override Guid ComponentGuid =>
            new Guid("f0456a61-c0c7-445a-b670-009b2ae5d1af");
    }

    // Who holds an element, read from its header without reserving it: nothing
    // changes in the project, so it refreshes like the other getters.
    public class GetReservationStatusOfElementsComponent : AbsTeamworkElementsComponent
    {
        public override string CommandName => "GetReservationStatusOfElements";

        public GetReservationStatusOfElementsComponent()
            : base(
                "ReservationStatusOfElements",
                "Returns the Teamwork reservation status of elements without reserving them: " +
                "whether each element is free, reserved by the current user or reserved by " +
                "another user, with the reserving user and the element's owner. " +
                "Fails in a solo project, where elements have no reservation.")
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to get the reservation status of.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Statuses",
                "Free: nobody has reserved the element. Editable: the current user has reserved it. " +
                "Locked: another user has reserved it (see ReservedByUserNames). " +
                "Null for an element that was not found.");

            OutIntegers(
                "ReservedByUserIds",
                "Teamwork user id of the user who has reserved each element (null when it is free), " +
                "as GrantElements expects it.");

            OutTexts(
                "ReservedByUserNames",
                "Name of the user who has reserved each element (null when it is free).");

            OutIntegers(
                "OwnerUserIds",
                "Teamwork user id of the owner of each element, usually the one who created it " +
                "(null when it has no owner).");

            OutTexts(
                "OwnerUserNames",
                "Name of the owner of each element (null when it has no owner).");

            OutErrorMessages(
                "Error message of each element (empty when its reservation status was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var statuses = new List<string>();
            var reservedByUserIds = new List<object>();
            var reservedByUserNames = new List<string>();
            var ownerUserIds = new List<object>();
            var ownerUserNames = new List<string>();
            var errorMessages = new List<string>();

            // One item per input element, in the input order: an element that was
            // not found gets nulls and its error message, so the lists stay aligned.
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "reservationStatuses"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    statuses.Add(null);
                    reservedByUserIds.Add(null);
                    reservedByUserNames.Add(null);
                    ownerUserIds.Add(null);
                    ownerUserNames.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                var reservedBy = item["reservedBy"];
                var owner = item["owner"];

                statuses.Add(item["status"]?.ToString());
                reservedByUserIds.Add(reservedBy?.Value<int?>("userId"));
                reservedByUserNames.Add(reservedBy?.Value<string>("userName"));
                ownerUserIds.Add(owner?.Value<int?>("userId"));
                ownerUserNames.Add(owner?.Value<string>("userName"));
                errorMessages.Add("");
            }

            da.SetDataList(
                0,
                statuses);

            da.SetDataList(
                1,
                reservedByUserIds);

            da.SetDataList(
                2,
                reservedByUserNames);

            da.SetDataList(
                3,
                ownerUserIds);

            da.SetDataList(
                4,
                ownerUserNames);

            da.SetDataList(
                5,
                errorMessages);
        }

        // Shares the icon of LockableObjectSetStatus until it gets its own, drawn
        // in IconSource/GetReservationStatusOfElements.svg.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLockableObjectSetStatus;

        public override Guid ComponentGuid =>
            new Guid("b84d1e27-5c6a-4f93-8e2d-7a1c9b3f6e54");
    }

    // Handing elements over changes the reservations on the Teamwork server, so
    // it runs only on the Execute button, like the lockable object set components.
    public class GrantElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "GrantElements";

        public GrantElementsComponent()
            : base(
                "GrantElements",
                "Hands over the reservation of elements to another Teamwork user, given by " +
                "user id, without a dialog. The elements must be reserved by the current user.",
                GroupNames.Teamwork)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to hand over; they must be reserved by the current user.");

            InInteger(
                "UserId",
                "Teamwork user id of the user to hand the elements over to, as TeamworkProjectInfo, " +
                "ReservationStatusOfElements and the conflicts of ReserveElements report users.");
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
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out int userId))
            {
                return;
            }

            if (userId <= 0)
            {
                this.AddError("The input UserId must be a positive Teamwork user id.");
                return;
            }

            var parameters = JObject.FromObject(elements);
            parameters["userId"] = userId;

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var result = ExecutionResult.Deserialize(response);
            if (!result.Success)
            {
                this.AddError(
                    $"Failed execution: {result.Message()}");
            }

            da.SetData(
                0,
                result.Message());
        }

        // Shares the icon of ReserveElements until it gets its own, drawn in
        // IconSource/GrantElements.svg.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ReserveElements;

        public override Guid ComponentGuid =>
            new Guid("3a7e5c19-2f8b-4d6e-a1c4-9e2b7f5d8a36");
    }
}