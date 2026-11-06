using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
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
                "Creates Project Info groups with the given names, at the end of the group list. Available from Archicad 30.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each new group, in order. A name must not be used by another group.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ProjectInfoGroupIds",
                "Identifier of each created group (null where the creation failed).");

            OutErrorMessages(
                "Error message of each group (empty when it was created).");
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

            var projectInfoGroups = new JArray();
            foreach (var name in names)
            {
                projectInfoGroups.Add(
                    new JObject { ["name"] = name });
            }

            SetCadValuesWithCreatedIds<ProjectInfoGroupGuid>(
                CommandName,
                new JObject { ["projectInfoGroups"] = projectInfoGroups },
                ToAddOn,
                da,
                "projectInfoGroupIdsOrErrors",
                "projectInfoGroupId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateProjectInfoGroups;

        public override Guid ComponentGuid =>
            new Guid("f736941f-dd3c-4d53-9409-c2c863c942c2");
    }
}
