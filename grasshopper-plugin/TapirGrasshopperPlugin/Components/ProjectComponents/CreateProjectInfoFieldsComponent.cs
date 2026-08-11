using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class CreateProjectInfoFieldsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateProjectInfoFields";

        public CreateProjectInfoFieldsComponent()
            : base(
                "CreateProjectInfoFields",
                "Create one or more custom project info fields.",
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

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Ids",
                "Id of each project info field of the project after the creation.");

            OutTexts(
                "Names",
                "Display name of each project info field.");

            OutTexts(
                "Values",
                "Value of each project info field.");
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

                input.ProjectInfoFields.Add(
                    new ProjectInfoFieldToCreate
                    {
                        ProjectInfoName = names[i],
                        ProjectInfoValue = value
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

            // The command answers with every project info field of the project,
            // including the ids Archicad generated for the new ones.
            var ids = new List<string>();
            var fieldNames = new List<string>();
            var fieldValues = new List<string>();

            if (response["fields"] is Newtonsoft.Json.Linq.JArray fields)
            {
                foreach (var field in fields)
                {
                    ids.Add(field["projectInfoId"]?.ToString() ?? "");
                    fieldNames.Add(field["projectInfoName"]?.ToString() ?? "");
                    fieldValues.Add(field["projectInfoValue"]?.ToString() ?? "");
                }
            }

            da.SetDataList(0, ids);
            da.SetDataList(1, fieldNames);
            da.SetDataList(2, fieldValues);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateProjectInfoFields;

        public override Guid ComponentGuid =>
            new Guid("6da24390-39b8-440b-84fb-7e3387681c39");
    }
}
