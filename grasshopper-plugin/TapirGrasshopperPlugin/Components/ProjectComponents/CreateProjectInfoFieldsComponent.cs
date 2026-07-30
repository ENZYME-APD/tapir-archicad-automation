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

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateProjectInfoFields;

        public override Guid ComponentGuid =>
            new Guid("6da24390-39b8-440b-84fb-7e3387681c39");
    }
}
