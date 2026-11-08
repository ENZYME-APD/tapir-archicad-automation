using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class MoveProjectInfoFieldsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "MoveProjectInfoFields";

        public MoveProjectInfoFieldsComponent()
            : base(
                "MoveProjectInfoFields",
                "Move the given fields of the Project Info groups to a group, before another field of it or to its end. A subfield of a calculated field moves with its parent field. Available from Archicad 30.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "FieldGuids",
                "Identifiers of the Project Info fields to move, in the order the moves are made, as the GetProjectInfoGroups component gives them (not the autotext keys).");

            InGenerics(
                "GroupGuids",
                "Identifier of the group to move each field to (input only 1 to move all into the same group). To reorder a field within its group, give its current group.");

            InGenerics(
                "BeforeFieldGuids",
                "Identifier of the field of the target group to move each field before (input only 1 to move all before the same field). Leave empty, or give a null item, to move to the end of the group.");

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each field (empty when it was moved successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> fieldWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> groupWrappers))
            {
                return;
            }

            if (groupWrappers.Count != 1 &&
                groupWrappers.Count != fieldWrappers.Count)
            {
                this.AddError(
                    "The size of the input GroupGuids must be 1 or equal to the size of the input FieldGuids.");
                return;
            }

            da.TryGetList(
                2,
                out List<GH_ObjectWrapper> beforeWrappers);
            beforeWrappers = beforeWrappers ?? new List<GH_ObjectWrapper>();

            if (beforeWrappers.Count > 1 &&
                beforeWrappers.Count != fieldWrappers.Count)
            {
                this.AddError(
                    "The size of the input BeforeFieldGuids must be 0, 1 or equal to the size of the input FieldGuids.");
                return;
            }

            var input = new MoveProjectInfoFieldsParameters
            {
                ProjectInfoFields = new List<MoveProjectInfoFieldParameter>()
            };

            for (var i = 0; i < fieldWrappers.Count; i++)
            {
                var fieldId =
                    GuidObject<ProjectInfoFieldGuid>.CreateFromWrapper(fieldWrappers[i]);
                if (fieldId == null)
                {
                    this.AddError(
                        "Invalid Project Info field identifier in the FieldGuids input.");
                    return;
                }

                var groupId = GuidObject<ProjectInfoGroupGuid>.CreateFromWrapper(
                    groupWrappers[groupWrappers.Count == 1 ? 0 : i]);
                if (groupId == null)
                {
                    this.AddError(
                        "Invalid Project Info group identifier in the GroupGuids input.");
                    return;
                }

                // No item, or a null item, means the end of the group.
                ProjectInfoFieldGuid beforeFieldId = null;
                var beforeWrapper = beforeWrappers.Count == 0
                    ? null
                    : beforeWrappers[beforeWrappers.Count == 1 ? 0 : i];
                if (beforeWrapper != null && beforeWrapper.Value != null)
                {
                    beforeFieldId =
                        GuidObject<ProjectInfoFieldGuid>.CreateFromWrapper(beforeWrapper);
                    if (beforeFieldId == null)
                    {
                        this.AddError(
                            "Invalid Project Info field identifier in the BeforeFieldGuids input.");
                        return;
                    }
                }

                input.ProjectInfoFields.Add(
                    new MoveProjectInfoFieldParameter
                    {
                        ProjectInfoFieldId = fieldId,
                        ProjectInfoGroupId = groupId,
                        BeforeProjectInfoFieldId = beforeFieldId
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveProjectInfoFields;

        public override Guid ComponentGuid =>
            new Guid("a1d7e4b9-6f28-4a05-9c3e-0b5d8f2a4e61");
    }
}
