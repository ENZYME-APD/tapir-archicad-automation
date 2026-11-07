using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class RenameProjectInfoGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameProjectInfoGroups";

        public RenameProjectInfoGroupsComponent()
            : base(
                "RenameProjectInfoGroups",
                "Renames Project Info groups. Available from Archicad 30.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ProjectInfoGroupIds",
                "Identifiers of the groups to rename (from GetProjectInfoGroups or CreateProjectInfoGroups).");

            InTexts(
                "Names",
                "New name of each group. A name must not be used by another group.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each group (empty when it was renamed).");
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

            if (!da.TryGetList(
                    1,
                    out List<string> names))
            {
                return;
            }

            if (names.Count != ids.Count)
            {
                this.AddError(
                    "The size of the input Names must be equal to the size of the input ProjectInfoGroupIds.");
                return;
            }

            var projectInfoGroups = new JArray();
            for (var i = 0; i < ids.Count; i++)
            {
                projectInfoGroups.Add(
                    new JObject
                    {
                        ["projectInfoGroupId"] = JObject.FromObject(ids[i].Id),
                        ["name"] = names[i]
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["projectInfoGroups"] = projectInfoGroups },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameProjectInfoGroups;

        public override Guid ComponentGuid =>
            new Guid("16d86a84-0a16-4fcb-8386-da6834e610c4");
    }
}
