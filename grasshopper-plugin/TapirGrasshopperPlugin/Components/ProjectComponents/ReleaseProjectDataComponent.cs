using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class ReleaseProjectDataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReleaseProjectData";

        public ReleaseProjectDataComponent()
            : base(
                "ReleaseProjectData",
                "Releases the project data with the given names in Teamwork.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the project data to release.");
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
            Properties.Resources.ReleaseProjectData;

        public override Guid ComponentGuid =>
            new Guid("eb7406f6-9685-4843-a381-669c943f44b4");
    }
}
