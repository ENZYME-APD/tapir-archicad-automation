using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class DeleteProjectInfoFieldsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteProjectInfoFields";

        public DeleteProjectInfoFieldsComponent()
            : base(
                "DeleteProjectInfoFields",
                "Delete one or more custom project info fields. Hardcoded fields cannot be deleted.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ProjectInfoIds",
                "Ids of the custom project info fields to delete (ids starting with 'autotext-').");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each project info field (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> ids))
            {
                return;
            }

            var input = new DeleteProjectInfoFieldsParameters
            {
                ProjectInfoIds = ids
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteProjectInfoFields;

        public override Guid ComponentGuid =>
            new Guid("33e568cc-995f-4e0e-b1f6-166f55e0c762");
    }
}
