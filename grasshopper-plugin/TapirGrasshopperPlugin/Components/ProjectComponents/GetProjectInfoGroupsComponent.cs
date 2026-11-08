using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetProjectInfoGroupsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetProjectInfoGroups";

        public GetProjectInfoGroupsComponent()
            : base(
                "GetProjectInfoGroups",
                "Retrieves the Project Info groups in their order, each with its fields: id, name, value and, " +
                "when exactly one project info autotext has the name of the field, the autotext key that " +
                "SetProjectInfoField takes. Available from Archicad 30.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ProjectInfoGroupIds",
                "Identifier of each Project Info group, in the order of the Project Info dialog.");

            OutTexts(
                "GroupNames",
                "Name of each group.");

            OutTextTree(
                "FieldIds",
                "Identifier (guid) of each field of the group (one branch per group), including the subfields of calculated fields.");

            OutTextTree(
                "FieldNames",
                "Name of each field as the Project Info dialog shows it (one branch per group).");

            OutTextTree(
                "FieldValues",
                "Value of each field (one branch per group).");

            OutTextTree(
                "ProjectInfoIds",
                "Autotext key of each field, as GetProjectInfoFields returns it and SetProjectInfoField takes it " +
                "(one branch per group). Null when no single project info autotext has the name of the field.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var groupIds = new List<ProjectInfoGroupGuid>();
            var groupNames = new List<string>();
            var fieldIds = new DataTree<string>();
            var fieldNames = new DataTree<string>();
            var fieldValues = new DataTree<string>();
            var projectInfoIds = new DataTree<string>();

            var groups = JsonOutputHelp.Items(
                response,
                "projectInfoGroups");
            for (var i = 0; i < groups.Count; i++)
            {
                var group = groups[i];
                var path = new GH_Path(i);
                fieldIds.EnsurePath(path);
                fieldNames.EnsurePath(path);
                fieldValues.EnsurePath(path);
                projectInfoIds.EnsurePath(path);

                groupIds.Add(
                    ProjectInfoGroupGuid.FromString(
                        JsonOutputHelp.WrappedGuidOf(
                            group,
                            "projectInfoGroupId")));
                groupNames.Add(group["name"]?.ToString());

                if (!(group["fields"] is JArray fields))
                {
                    continue;
                }

                foreach (var field in fields)
                {
                    fieldIds.Add(
                        JsonOutputHelp.WrappedGuidOf(
                            field,
                            "projectInfoFieldId"),
                        path);
                    fieldNames.Add(
                        field["name"]?.ToString(),
                        path);
                    fieldValues.Add(
                        field["value"]?.ToString(),
                        path);
                    projectInfoIds.Add(
                        field["projectInfoId"]?.ToString(),
                        path);
                }
            }

            da.SetDataList(0, groupIds);
            da.SetDataList(1, groupNames);
            da.SetDataTree(2, fieldIds);
            da.SetDataTree(3, fieldNames);
            da.SetDataTree(4, fieldValues);
            da.SetDataTree(5, projectInfoIds);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetProjectInfoGroups;

        public override Guid ComponentGuid =>
            new Guid("82b3f866-5845-4f35-9f21-5929da3fc895");
    }
}
