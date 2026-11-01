using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class ReserveProjectDataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReserveProjectData";

        public ReserveProjectDataComponent()
            : base(
                "ReserveProjectData",
                "Reserves the project data with the given names in Teamwork, so that other users cannot change it. " +
                "Returns the users who have already reserved some of it.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the project data to reserve.");
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult),
                ExecutionResult.Doc);

            OutTexts(
                "ConflictNames",
                "Names of the project data that could not be reserved because another user has reserved it.");

            OutIntegers(
                "ConflictUserIds",
                "Teamwork user id of the user who has reserved each conflicting project data (GrantProjectData takes it).");

            OutTexts(
                "ConflictUserNames",
                "Name of the user who has reserved each conflicting project data.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ProjectDataHelp.TryGetNames(
                    da,
                    out var parameters))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var conflictNames = new List<string>();
            var conflictUserIds = new List<int?>();
            var conflictUserNames = new List<string>();
            foreach (var conflict in JsonOutputHelp.Items(
                         response,
                         "conflicts"))
            {
                conflictNames.Add(conflict["name"]?.ToString());
                conflictUserIds.Add(conflict["user"]?.Value<int?>("userId"));
                conflictUserNames.Add(conflict["user"]?["userName"]?.ToString());
            }

            da.SetData(
                0,
                ProjectComponentHelp.ExecutionResultMessage(response["executionResult"]));
            da.SetDataList(1, conflictNames);
            da.SetDataList(2, conflictUserIds);
            da.SetDataList(3, conflictUserNames);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ReserveProjectData;

        public override Guid ComponentGuid =>
            new Guid("1aa5b02c-31f8-4bfd-9d12-be2b0070e5c2");
    }
}
