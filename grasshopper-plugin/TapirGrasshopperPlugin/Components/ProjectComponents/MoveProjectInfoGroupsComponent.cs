using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class MoveProjectInfoGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "MoveProjectInfoGroups";

        public MoveProjectInfoGroupsComponent()
            : base(
                "MoveProjectInfoGroups",
                "Move the given Project Info groups before another group, or to the end of the group list. Available from Archicad 30.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "GroupGuids",
                "Identifiers of the Project Info groups to move, in the order the moves are made.");

            InGenerics(
                "BeforeGroupGuids",
                "Identifier of the group to move each group before (input only 1 to move all before the same group). Leave empty, or give a null item, to move to the end of the group list.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each Project Info group (empty when it was moved successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> groupWrappers))
            {
                return;
            }

            da.TryGetList(
                1,
                out List<GH_ObjectWrapper> beforeWrappers);
            beforeWrappers = beforeWrappers ?? new List<GH_ObjectWrapper>();

            if (beforeWrappers.Count > 1 &&
                beforeWrappers.Count != groupWrappers.Count)
            {
                this.AddError(
                    "The size of the input BeforeGroupGuids must be 0, 1 or equal to the size of the input GroupGuids.");
                return;
            }

            var input = new MoveProjectInfoGroupsParameters
            {
                ProjectInfoGroups = new List<MoveProjectInfoGroupParameter>()
            };

            for (var i = 0; i < groupWrappers.Count; i++)
            {
                var groupId =
                    GuidObject<ProjectInfoGroupGuid>.CreateFromWrapper(groupWrappers[i]);
                if (groupId == null)
                {
                    this.AddError(
                        "Invalid Project Info group identifier in the GroupGuids input.");
                    return;
                }

                // No item, or a null item, means the end of the group list.
                ProjectInfoGroupGuid beforeGroupId = null;
                var beforeWrapper = beforeWrappers.Count == 0
                    ? null
                    : beforeWrappers[beforeWrappers.Count == 1 ? 0 : i];
                if (beforeWrapper != null && beforeWrapper.Value != null)
                {
                    beforeGroupId =
                        GuidObject<ProjectInfoGroupGuid>.CreateFromWrapper(beforeWrapper);
                    if (beforeGroupId == null)
                    {
                        this.AddError(
                            "Invalid Project Info group identifier in the BeforeGroupGuids input.");
                        return;
                    }
                }

                input.ProjectInfoGroups.Add(
                    new MoveProjectInfoGroupParameter
                    {
                        ProjectInfoGroupId = groupId,
                        BeforeProjectInfoGroupId = beforeGroupId
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveProjectInfoGroups;

        public override Guid ComponentGuid =>
            new Guid("e92b6d13-4a7c-4e58-a3f1-5b8d2c7e9a46");
    }
}
