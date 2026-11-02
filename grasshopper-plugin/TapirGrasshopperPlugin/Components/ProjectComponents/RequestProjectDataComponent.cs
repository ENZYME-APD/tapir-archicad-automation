using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class RequestProjectDataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RequestProjectData";

        public RequestProjectDataComponent()
            : base(
                "RequestProjectData",
                "Requests the project data with the given names in Teamwork from the users who have reserved it, " +
                "with an optional message.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the project data to request from the users who have reserved them.");

            InText(
                "MailText",
                "The message sent with the request. Empty when unconnected.");

            SetOptionality(1);
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
                    out var parameters))
            {
                return;
            }

            da.TryAddOptional<string>(
                1,
                parameters,
                "mailText");

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
            Properties.Resources.RequestProjectData;

        public override Guid ComponentGuid =>
            new Guid("3c8fbc94-0ee3-4206-b159-605a04ef1854");
    }
}
