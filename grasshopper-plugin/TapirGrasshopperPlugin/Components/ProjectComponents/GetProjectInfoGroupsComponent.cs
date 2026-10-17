using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Linq;
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
                "Get the Project Info groups in their order, each with its fields. Available from Archicad 30.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "GroupGuids",
                "Identifiers of the Project Info groups.");

            OutTexts(
                "GroupNames",
                "Names of the Project Info groups.");

            OutGenericTree(
                "FieldGuids",
                "Identifiers of the fields of each group (one branch per group), including the subfields of calculated fields.");

            OutTextTree(
                "FieldNames",
                "Names of the fields of each group (one branch per group).");

            OutTextTree(
                "FieldValues",
                "Values of the fields of each group (one branch per group).");

            OutTextTree(
                "FieldIds",
                "Autotext key of each field, the id the SetProjectInfoFields component takes (one branch per group); empty when more than one project info autotext has the field's name.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<GetProjectInfoGroupsResponse>,
                    out GetProjectInfoGroupsResponse response))
            {
                return;
            }

            var groups = response.ProjectInfoGroups ?? new List<ProjectInfoGroupDetails>();

            da.SetDataList(
                0,
                groups.Select(g => g.ProjectInfoGroupId));

            da.SetDataList(
                1,
                groups.Select(g => g.Name));

            var fieldGuids = new GH_Structure<GH_ObjectWrapper>();
            var fieldNames = new GH_Structure<GH_String>();
            var fieldValues = new GH_Structure<GH_String>();
            var fieldIds = new GH_Structure<GH_String>();

            for (var i = 0; i < groups.Count; i++)
            {
                var path = new GH_Path(i);
                fieldGuids.EnsurePath(path);
                fieldNames.EnsurePath(path);
                fieldValues.EnsurePath(path);
                fieldIds.EnsurePath(path);

                foreach (var field in groups[i].Fields ?? new List<ProjectInfoGroupField>())
                {
                    fieldGuids.Append(
                        new GH_ObjectWrapper(field.ProjectInfoFieldId),
                        path);
                    fieldNames.Append(
                        new GH_String(field.Name ?? ""),
                        path);
                    fieldValues.Append(
                        new GH_String(field.Value ?? ""),
                        path);
                    fieldIds.Append(
                        new GH_String(field.ProjectInfoId ?? ""),
                        path);
                }
            }

            da.SetDataTree(2, fieldGuids);
            da.SetDataTree(3, fieldNames);
            da.SetDataTree(4, fieldValues);
            da.SetDataTree(5, fieldIds);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetProjectInfoGroups;

        public override Guid ComponentGuid =>
            new Guid("26c8fe00-287a-4694-9118-e8294d18f27b");
    }
}
