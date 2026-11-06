using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GrantProjectDataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "GrantProjectData";

        public GrantProjectDataComponent()
            : base(
                "GrantProjectData",
                "Grants the project data with the given names, reserved by the current user, to another Teamwork user.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the project data, reserved by the current user, to grant.");

            inManager.AddIntegerParameter(
                "ToUserId",
                "ToUserId",
                ("The Teamwork user id of the user who gets the project data, for example a TeamworkOwnerUserIds " +
                 "item of GetProjectDataKeys or a ConflictUserIds item of ReserveProjectData.").WithTypeName("integerItem"),
                GH_ParamAccess.item);
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
            if (!ProjectDataHelp.TryGetNames(
                    da,
                    out var parameters) ||
                !da.TryAddOptional<int>(
                    1,
                    parameters,
                    "toUserId"))
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

            da.SetData(
                0,
                ProjectComponentHelp.ExecutionResultMessage(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GrantProjectData;

        public override Guid ComponentGuid =>
            new Guid("fec39077-b862-4280-b8c4-a6bf0312c837");
    }
}
