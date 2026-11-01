using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class DeleteProjectInfoGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteProjectInfoGroups";

        public DeleteProjectInfoGroupsComponent()
            : base(
                "DeleteProjectInfoGroups",
                "Deletes Project Info groups. A group that contains fixed (built-in) fields cannot be deleted. " +
                "Available from Archicad 30.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ProjectInfoGroupIds",
                "Identifiers of the groups to delete (from GetProjectInfoGroups or CreateProjectInfoGroups).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each group (empty when it was deleted).");
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

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["projectInfoGroups"] = JArray.FromObject(ids) },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteProjectInfoGroups;

        public override Guid ComponentGuid =>
            new Guid("0f9abdc2-dcdf-483b-ba02-599752931453");
    }
}
