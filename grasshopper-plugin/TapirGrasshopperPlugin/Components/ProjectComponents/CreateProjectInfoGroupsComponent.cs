using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class CreateProjectInfoGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateProjectInfoGroups";

        public CreateProjectInfoGroupsComponent()
            : base(
                "CreateProjectInfoGroups",
                "Create Project Info groups with the given names, at the end of the group list. Available from Archicad 30.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the Project Info groups to create. A name must not be used by another group.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "GroupGuids",
                "Identifier of each created Project Info group.");

            OutErrorMessages(
                "Error message of each Project Info group (empty when it was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return;
            }

            var input = new CreateProjectInfoGroupsParameters
            {
                ProjectInfoGroups = new List<ProjectInfoGroupToCreate>()
            };

            foreach (var name in names)
            {
                if (string.IsNullOrEmpty(name))
                {
                    this.AddError(
                        "Every item of the Names input needs a name.");
                    return;
                }

                input.ProjectInfoGroups.Add(
                    new ProjectInfoGroupToCreate
                    {
                        Name = name
                    });
            }

            SetCadValuesWithCreatedIds<ProjectInfoGroupGuid>(
                CommandName,
                input,
                ToAddOn,
                da,
                "projectInfoGroupIdsOrErrors",
                "projectInfoGroupId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateProjectInfoGroups;

        public override Guid ComponentGuid =>
            new Guid("7b3d2a41-5c6e-4f80-9a2b-1d4e6f8a0c23");
    }
}
