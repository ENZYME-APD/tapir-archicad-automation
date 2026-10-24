using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class DeleteProjectInfoFieldsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteProjectInfoFields";

        public DeleteProjectInfoFieldsComponent()
            : base(
                "DeleteProjectInfoFields",
                "Delete one or more custom project info fields, by their autotext keys or, from Archicad 30, by their identifiers in the Project Info groups. Hardcoded fields cannot be deleted.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ProjectInfoIds",
                "Autotext keys of the custom project info fields to delete (ids starting with 'autotext-'). On Archicad 30 a key is matched to the field by its name, so it fails when more than one field has the name; delete such a field through FieldGuids instead.");

            InGenerics(
                "FieldGuids",
                "Identifiers of the custom fields to delete, as the GetProjectInfoGroups component gives them. Available from Archicad 30.");

            SetOptionality(0);
            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each project info field (empty when it succeeded): first one per item of ProjectInfoIds, then one per item of FieldGuids.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGetList(
                0,
                out List<string> ids);
            ids = ids ?? new List<string>();

            da.TryGetList(
                1,
                out List<GH_ObjectWrapper> fieldWrappers);
            fieldWrappers = fieldWrappers ?? new List<GH_ObjectWrapper>();

            if (ids.Count == 0 && fieldWrappers.Count == 0)
            {
                this.AddError(
                    "Give the fields to delete in the ProjectInfoIds or the FieldGuids input.");
                return;
            }

            var input = new DeleteProjectInfoFieldsParameters();

            if (ids.Count > 0)
            {
                input.ProjectInfoIds = ids;
            }

            if (fieldWrappers.Count > 0)
            {
                input.ProjectInfoFieldIds = new List<ProjectInfoFieldGuidWrapper>();
                foreach (var wrapper in fieldWrappers)
                {
                    var fieldId = GuidObject<ProjectInfoFieldGuid>.CreateFromWrapper(wrapper);
                    if (fieldId == null)
                    {
                        this.AddError(
                            "Invalid Project Info field identifier in the FieldGuids input.");
                        return;
                    }

                    input.ProjectInfoFieldIds.Add(
                        new ProjectInfoFieldGuidWrapper
                        {
                            ProjectInfoFieldId = fieldId
                        });
                }
            }

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
