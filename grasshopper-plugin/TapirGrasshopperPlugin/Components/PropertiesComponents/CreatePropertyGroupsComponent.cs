using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class CreatePropertyGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreatePropertyGroups";

        public CreatePropertyGroupsComponent()
            : base(
                "CreatePropertyGroups",
                "Create Custom Property Groups based on the given parameters.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the property groups to create.");

            InTexts(
                "Descriptions",
                "Descriptions of the property groups (input 0, 1 or the same number as Names).");

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
                out List<string> descriptions);
            descriptions = descriptions ?? new List<string>();

            if (descriptions.Count > 1 &&
                descriptions.Count != names.Count)
            {
                this.AddError(
                    "The size of the input Descriptions must be 0, 1 or equal to the size of the input Names.");
                return;
            }

            var input = new CreatePropertyGroupsParameters
            {
                PropertyGroups = new List<PropertyGroupArrayItem>()
            };

            for (var i = 0; i < names.Count; i++)
            {
                string description = null;
                if (descriptions.Count == 1)
                {
                    description = descriptions[0];
                }
                else if (descriptions.Count > i)
                {
                    description = descriptions[i];
                }

                input.PropertyGroups.Add(
                    new PropertyGroupArrayItem
                    {
                        PropertyGroup = new PropertyGroupToCreate
                        {
                            Name = names[i],
                            Description = description
                        }
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreatePropertyGroups;

        public override Guid ComponentGuid =>
            new Guid("655c91f2-e355-48dd-b2da-9349f0c41d22");
    }
}
