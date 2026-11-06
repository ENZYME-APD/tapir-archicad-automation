using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class RenameProjectInfoFieldsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameProjectInfoFields";

        public RenameProjectInfoFieldsComponent()
            : base(
                "RenameProjectInfoFields",
                "Change the name of the given custom fields of the Project Info groups. Fixed (built-in) fields cannot be renamed. Available from Archicad 30.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "FieldGuids",
                "Identifiers of the Project Info fields to rename, as the GetProjectInfoGroups component gives them (not the autotext keys).");

            InTexts(
                "Names",
                "New name of each field. A name must not be used by another field.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each field (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> fieldWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> names))
            {
                return;
            }

            if (names.Count != fieldWrappers.Count)
            {
                this.AddError(
                    "The size of the inputs FieldGuids and Names must be equal.");
                return;
            }

            var input = new RenameProjectInfoFieldsParameters
            {
                ProjectInfoFields = new List<RenameProjectInfoFieldParameter>()
            };

            for (var i = 0; i < fieldWrappers.Count; i++)
            {
                var fieldId =
                    GuidObject<ProjectInfoFieldGuid>.CreateFromWrapper(fieldWrappers[i]);
                if (fieldId == null)
                {
                    this.AddError(
                        "Invalid Project Info field identifier in the FieldGuids input.");
                    return;
                }

                if (string.IsNullOrEmpty(names[i]))
                {
                    this.AddError(
                        "Every item of the Names input needs a name.");
                    return;
                }

                input.ProjectInfoFields.Add(
                    new RenameProjectInfoFieldParameter
                    {
                        ProjectInfoFieldId = fieldId,
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
            Properties.Resources.RenameProjectInfoFields;

        public override Guid ComponentGuid =>
            new Guid("3f8c5e27-9d1b-4c63-b2a8-6e4f1d9c7b30");
    }
}
