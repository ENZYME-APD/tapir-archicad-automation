using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class CreateProjectInfoFieldsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateProjectInfoFields";

        public CreateProjectInfoFieldsComponent()
            : base(
                "CreateProjectInfoFields",
                "Create one or more custom project info fields. From Archicad 30 the Project Info group of each field can be given; without it the field goes to the first group.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Display names of the project info fields to create.");

            InTexts(
                "Values",
                "Initial values of the project info fields (input only 1 to use the same value for all, or leave empty).");

            InGenerics(
                "GroupGuids",
                "Identifier of the Project Info group to create each field in, as the GetProjectInfoGroups component gives them (input only 1 to create all in the same group, or leave empty for the first group). Available from Archicad 30; on older versions giving a group is an error.");

            SetOptionality(1);
            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Ids",
                "Autotext key of each created project info field, the id the SetProjectInfoFields component takes; empty when the field was not created or, on Archicad 30, when more than one project info autotext has its name.");

            OutTexts(
                "Names",
                "Display name of each created project info field (empty when it was not created).");

            OutTexts(
                "Values",
                "Value of each created project info field (empty when it was not created).");

            OutGenerics(
                "FieldGuids",
                "Identifier of each created field in the Project Info groups, as the GetProjectInfoGroups component gives them. Available from Archicad 30, null otherwise.");

            OutErrorMessages(
                "Error message of each project info field (empty when it was created successfully).");
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

            da.TryGetList(
                1,
                out List<string> values);
            values = values ?? new List<string>();

            if (values.Count > 1 &&
                values.Count != names.Count)
            {
                this.AddError(
                    "The size of the input Values must be 0, 1 or equal to the size of the input Names.");
                return;
            }

            da.TryGetList(
                2,
                out List<GH_ObjectWrapper> groupWrappers);
            groupWrappers = groupWrappers ?? new List<GH_ObjectWrapper>();

            if (groupWrappers.Count > 1 &&
                groupWrappers.Count != names.Count)
            {
                this.AddError(
                    "The size of the input GroupGuids must be 0, 1 or equal to the size of the input Names.");
                return;
            }

            var input = new CreateProjectInfoFieldsParameters
            {
                ProjectInfoFields = new List<ProjectInfoFieldToCreate>()
            };

            for (var i = 0; i < names.Count; i++)
            {
                string value = null;
                if (values.Count == 1)
                {
                    value = values[0];
                }
                else if (values.Count > i)
                {
                    value = values[i];
                }

                // No item, or a null item, means the first group.
                ProjectInfoGroupGuid groupId = null;
                var groupWrapper = groupWrappers.Count == 0
                    ? null
                    : groupWrappers[groupWrappers.Count == 1 ? 0 : i];
                if (groupWrapper != null && groupWrapper.Value != null)
                {
                    groupId =
                        GuidObject<ProjectInfoGroupGuid>.CreateFromWrapper(groupWrapper);
                    if (groupId == null)
                    {
                        this.AddError(
                            "Invalid Project Info group identifier in the GroupGuids input.");
                        return;
                    }
                }

                input.ProjectInfoFields.Add(
                    new ProjectInfoFieldToCreate
                    {
                        ProjectInfoName = names[i],
                        ProjectInfoValue = value,
                        ProjectInfoGroupId = groupId
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    Newtonsoft.Json.Linq.JObject.FromObject(input),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            // The command answers with one item per requested field: the created
            // field, or an error. The outputs stay aligned with the Names input.
            var ids = new List<string>();
            var fieldNames = new List<string>();
            var fieldValues = new List<string>();
            var fieldGuids = new List<ProjectInfoFieldGuid>();
            var errorMessages = new List<string>();

            if (response["fields"] is Newtonsoft.Json.Linq.JArray fields)
            {
                foreach (var field in fields)
                {
                    var error = field?["error"];
                    if (error != null)
                    {
                        ids.Add("");
                        fieldNames.Add("");
                        fieldValues.Add("");
                        fieldGuids.Add(null);
                        errorMessages.Add(
                            error["message"]?.ToString() ?? "Unknown error.");
                        continue;
                    }

                    ids.Add(field["projectInfoId"]?.ToString() ?? "");
                    fieldNames.Add(field["projectInfoName"]?.ToString() ?? "");
                    fieldValues.Add(field["projectInfoValue"]?.ToString() ?? "");
                    fieldGuids.Add(
                        field["projectInfoFieldId"]?.ToObject<ProjectInfoFieldGuid>());
                    errorMessages.Add("");
                }
            }

            da.SetDataList(0, ids);
            da.SetDataList(1, fieldNames);
            da.SetDataList(2, fieldValues);
            da.SetDataList(3, fieldGuids);
            da.SetDataList(4, errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateProjectInfoFields;

        public override Guid ComponentGuid =>
            new Guid("6da24390-39b8-440b-84fb-7e3387681c39");
    }
}
