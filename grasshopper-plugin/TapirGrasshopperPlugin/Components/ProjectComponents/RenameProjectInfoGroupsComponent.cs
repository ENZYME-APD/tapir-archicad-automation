using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class RenameProjectInfoGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameProjectInfoGroups";

        public RenameProjectInfoGroupsComponent()
            : base(
                "RenameProjectInfoGroups",
                "Change the name of the given Project Info groups. Available from Archicad 30.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "GroupGuids",
                "Identifiers of the Project Info groups to rename.");

            InTexts(
                "Names",
                "New name of each Project Info group. A name must not be used by another group.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each Project Info group (empty when it succeeded).");
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

            if (!da.TryGetList(
                    1,
                    out List<string> names))
            {
                return;
            }

            if (names.Count != groupWrappers.Count)
            {
                this.AddError(
                    "The size of the inputs GroupGuids and Names must be equal.");
                return;
            }

            var input = new RenameProjectInfoGroupsParameters
            {
                ProjectInfoGroups = new List<RenameProjectInfoGroupParameter>()
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

                if (string.IsNullOrEmpty(names[i]))
                {
                    this.AddError(
                        "Every item of the Names input needs a name.");
                    return;
                }

                input.ProjectInfoGroups.Add(
                    new RenameProjectInfoGroupParameter
                    {
                        ProjectInfoGroupId = groupId,
                        Name = names[i]
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameProjectInfoGroups;

        public override Guid ComponentGuid =>
            new Guid("c4e19f72-8b3a-4d15-b6c7-2f9e0a1d3b58");
    }
}
