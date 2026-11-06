using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
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
                "Moves Project Info groups before another group, or to the end of the group list when no other " +
                "group is given. Available from Archicad 30.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ProjectInfoGroupIds",
                "Identifiers of the groups to move, in the order the moves are made.");

            InGenerics(
                "BeforeProjectInfoGroupIds",
                "The group to move each group before (input only 1 to use the same group for all). " +
                "Leave it empty, or give an empty item, to move the group to the end of the group list.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each move (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> wrappers) ||
                !this.TryGetProjectInfoGroupIds(
                    wrappers,
                    "ProjectInfoGroupIds",
                    out var ids))
            {
                return;
            }

            da.TryGetList(
                1,
                out List<GH_ObjectWrapper> beforeWrappers);

            // An empty item moves its group to the end of the list, so the
            // targets are read one by one instead of all being required.
            var beforeIds = new List<ProjectInfoGroupGuid>();
            foreach (var wrapper in beforeWrappers)
            {
                if (wrapper == null ||
                    wrapper.Value == null ||
                    (wrapper.Value is GH_String text && string.IsNullOrWhiteSpace(text.Value)))
                {
                    beforeIds.Add(null);
                    continue;
                }

                var beforeId = GuidObject<ProjectInfoGroupGuid>.CreateFromWrapper(wrapper);
                if (beforeId == null)
                {
                    this.AddError(
                        "Invalid Project Info group identifier in the BeforeProjectInfoGroupIds input.");
                    return;
                }

                beforeIds.Add(beforeId);
            }

            if (beforeIds.Count > 0)
            {
                beforeIds = this.ExpandToCount(
                    beforeIds,
                    ids.Count,
                    "BeforeProjectInfoGroupIds",
                    "ProjectInfoGroupIds");
                if (beforeIds == null)
                {
                    return;
                }
            }

            var projectInfoGroups = new JArray();
            for (var i = 0; i < ids.Count; i++)
            {
                var projectInfoGroup = new JObject
                {
                    ["projectInfoGroupId"] = JObject.FromObject(ids[i].Id)
                };

                if (beforeIds.Count > 0 &&
                    beforeIds[i] != null)
                {
                    projectInfoGroup["beforeProjectInfoGroupId"] =
                        JObject.FromObject(beforeIds[i]);
                }

                projectInfoGroups.Add(projectInfoGroup);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["projectInfoGroups"] = projectInfoGroups },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveProjectInfoGroups;

        public override Guid ComponentGuid =>
            new Guid("0022c3ff-55e8-475d-b8d5-55e8ef5957a8");
    }
}
